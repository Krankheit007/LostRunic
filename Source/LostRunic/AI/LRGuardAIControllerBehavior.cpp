/**
 * @file LRGuardAIControllerBehavior.cpp
 * @brief 守卫控制器行为实现：行为进出与移动驱动、警戒数据变化到 BehaviorChanged 的分派（仅实际变化时广播）、击退晕眩覆盖、巡逻与诊断。
 *
 * 关联文件：LRGuardAIController.h；所属领域：AI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "AI/LRGuardAIController.h"

#include "AI/LRAlertComponent.h"
#include "AI/LRGuardCharacter.h"
#include "Components/StateTreeAIComponent.h"
#include "Core/LRGameplayTags.h"
#include "Core/LRLog.h"
#include "Data/LRGuardTuning.h"
#include "Data/LRStateTuning.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "TimerManager.h"

/**
 * @brief 进入指定守卫行为，设置移动速度、焦点、导航目标；眩晕中仅接受 Stunned。
 * @param behavior 要进入或退出的守卫 StateTree 行为状态。
 */
void ALRGuardAIController::EnterBehavior(const ELRGuardBehaviorState behavior)
{
	if (bStunned && behavior != ELRGuardBehaviorState::Stunned)
	{
		ActiveBehavior = ELRGuardBehaviorState::Stunned;
		return;
	}
	ActiveBehavior = behavior;
	ALRGuardCharacter* guard = Cast<ALRGuardCharacter>(GetPawn());
	if (!guard || !Alert.IsValid())
	{
		return;
	}
	UCharacterMovementComponent* movement = guard->GetCharacterMovement();
	if (behavior == ELRGuardBehaviorState::Chase)
	{
		movement->MaxWalkSpeed = GetEffectiveTuning().ChaseSpeed;
		SetFocus(Alert->GetTargetActor());
		MoveToActor(Alert->GetTargetActor(), GetEffectiveTuning().CaptureRadius);
	}
	else if (behavior == ELRGuardBehaviorState::Investigate)
	{
		movement->MaxWalkSpeed = GetEffectiveTuning().InvestigateSpeed;
		MoveToLocation(Alert->GetLastDisturbanceLocation(), GetEffectiveTuning().MoveAcceptanceRadius);
	}
	else if (behavior == ELRGuardBehaviorState::Search)
	{
		// 抵达观察与自然衰减由 AlertComponent 的观察计时与衰减计时驱动，不再使用固定搜索时长。
		StopMovement();
		SetFocalPoint(Alert->GetLastDisturbanceLocation());
	}
	else if (behavior == ELRGuardBehaviorState::Suspicious)
	{
		StopMovement();
		SetFocalPoint(Alert->GetLastDisturbanceLocation());
	}
	else if (behavior == ELRGuardBehaviorState::Stunned)
	{
		StopMovement();
		ClearFocus(EAIFocusPriority::Gameplay);
	}
	else
	{
		movement->MaxWalkSpeed = GetEffectiveTuning().InvestigateSpeed;
		StartPatrolMove();
	}
}

/**
 * @brief 退出指定守卫行为并清理该状态拥有的导航、焦点或计时器。
 * @param behavior 要进入或退出的守卫 StateTree 行为状态。
 */
void ALRGuardAIController::ExitBehavior(const ELRGuardBehaviorState behavior)
{
	StopMovement();
	ClearFocus(EAIFocusPriority::Gameplay);
}

/**
 * @brief 处理 On Move Completed 事件：调查抵达转入 Search（开始观察），巡逻点到达续走下一段。
 * @param requestId 稳定标识 `requestId`；用于内容查询和存档，不依赖显示名或数组序号。
 * @param result 本次领域操作的结构化数据 `result`；字段语义由对应 USTRUCT 定义。
 */
void ALRGuardAIController::OnMoveCompleted(const FAIRequestID requestId, const FPathFollowingResult& result)
{
	Super::OnMoveCompleted(requestId, result);
	if (!result.IsSuccess() || !Alert.IsValid())
	{
		return;
	}
	if (ActiveBehavior == ELRGuardBehaviorState::Investigate)
	{
		Alert->MarkInvestigationReached();
	}
	else if (ActiveBehavior == ELRGuardBehaviorState::IdlePatrol)
	{
		++PatrolIndex;
		StartPatrolMove();
	}
}

/**
 * @brief 处理 Handle Alert Changed 事件：仅表示感知/警戒数据变化；只有当权威解析结果实际变化时才广播 BehaviorChanged，避免衰减计时等数值变化导致 StateTree 无意义重入。
 * @param previousLevel 本次操作使用的计数、增量或索引 `previousLevel`；由函数校验合法范围。
 * @param currentLevel 本次操作使用的计数、增量或索引 `currentLevel`；由函数校验合法范围。
 * @param currentState 本次操作使用的 `currentState` 枚举或模式值。
 * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
 * @param disturbanceLocation 空间值 `disturbanceLocation`；距离和位置使用 Unreal 厘米单位。
 */
void ALRGuardAIController::HandleAlertChanged(const int32 previousLevel, const int32 currentLevel,
	const ELRGuardBehaviorState currentState, const FGameplayTag reason, const FVector disturbanceLocation)
{
	const ELRGuardBehaviorState resolved = GetResolvedBehavior();
	if (resolved == ActiveBehavior)
	{
		// 同状态 Investigate 的数据级重定位（新调查点），不发行为事件。
		if (resolved == ELRGuardBehaviorState::Investigate)
		{
			EnterBehavior(ELRGuardBehaviorState::Investigate);
		}
		return;
	}
	if (StateTreeAI->IsRunning())
	{
		StateTreeAI->SendStateTreeEvent(LRGameplayTags::AIEventBehaviorChanged, FConstStructView(), FName());
	}
	else
	{
		EnterBehavior(resolved);
	}
}

/**
 * @brief 处理 Handle Knockback 事件：进入 Stunned 覆盖（停止移动、清除焦点），按 Courage 击退时长计时恢复。
 * @param direction 击退方向 `direction`；仅用于诊断。
 */
void ALRGuardAIController::HandleKnockback(const FVector direction)
{
	if (bStunned)
	{
		return;
	}
	bStunned = true;
	StopMovement();
	ClearFocus(EAIFocusPriority::Gameplay);
	UE_LOG(LogLostRunicAI, Display, TEXT("Guard=%s stunned for %.2fs direction=%s"), *GetNameSafe(GetPawn()),
		StateTuning ? StateTuning->CourageKnockbackDurationSeconds : 0.6f, *direction.ToCompactString());
	if (StateTreeAI->IsRunning())
	{
		StateTreeAI->SendStateTreeEvent(LRGameplayTags::AIEventBehaviorChanged, FConstStructView(), FName());
	}
	else
	{
		EnterBehavior(ELRGuardBehaviorState::Stunned);
	}
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(StunTimer, this, &ALRGuardAIController::HandleStunEnd,
			StateTuning ? StateTuning->CourageKnockbackDurationSeconds : 0.6f, false);
	}
}

/**
 * @brief 眩晕计时结束后按当前警戒与视线重新解析行为并广播恢复事件；感知与警戒在眩晕期间持续运行。
 */
void ALRGuardAIController::HandleStunEnd()
{
	bStunned = false;
	const ELRGuardBehaviorState resolved = GetResolvedBehavior();
	if (StateTreeAI->IsRunning())
	{
		StateTreeAI->SendStateTreeEvent(LRGameplayTags::AIEventBehaviorChanged, FConstStructView(), FName());
	}
	else
	{
		EnterBehavior(resolved);
	}
}

/**
 * @brief 开始 Start Patrol Move 流程，建立本次操作拥有的状态、委托或计时器。
 */
void ALRGuardAIController::StartPatrolMove()
{
	ALRGuardCharacter* guard = Cast<ALRGuardCharacter>(GetPawn());
	if (!guard || guard->GetPatrolPointCount() == 0)
	{
		StopMovement();
		return;
	}
	PatrolIndex %= guard->GetPatrolPointCount();
	MoveToActor(guard->GetPatrolPoint(PatrolIndex), GetEffectiveTuning().MoveAcceptanceRadius);
}

/**
 * @brief 输出守卫行为、警戒值、观察/眩晕状态和最后异常点，并按调试开关绘制视野与听觉范围。
 */
void ALRGuardAIController::LogAndDrawDiagnostics() const
{
	const APawn* guard = GetPawn();
	if (!guard || !Alert.IsValid())
	{
		return;
	}
	const ULRGuardTuning& tuning = GetEffectiveTuning();
	UE_LOG(LogLostRunicAI, Display,
		TEXT("Guard=%s Alert=%d ResolvedState=%d Observing=%d Stunned=%d Target=%s Reason=%s Location=%s"),
		*GetNameSafe(guard), Alert->GetAlertLevel(), static_cast<int32>(GetResolvedBehavior()),
		Alert->IsObserving() ? 1 : 0, bStunned ? 1 : 0,
		*GetNameSafe(Alert->GetTargetActor()), *Alert->GetLastReason().GetTagName().ToString(),
		*Alert->GetLastDisturbanceLocation().ToCompactString());
	const FVector origin = guard->GetActorLocation();
	DrawDebugCone(GetWorld(), origin, guard->GetActorForwardVector(), tuning.SightRadius,
		FMath::DegreesToRadians(tuning.SightConeDegrees * 0.5f), FMath::DegreesToRadians(tuning.SightConeDegrees * 0.5f),
		16, FColor::Yellow, false, 5.0f);
	DrawDebugSphere(GetWorld(), origin, tuning.MaxHearingRange, 32, FColor::Cyan, false, 5.0f);
	DrawDebugSphere(GetWorld(), origin, tuning.CaptureRadius, 16, FColor::Red, false, 5.0f);
}
