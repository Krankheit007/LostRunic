/**
 * @file LRGuardAIController.cpp
 * @brief 把 AI Perception 的 Sight/Hearing 事件转换为警戒原因标签，并驱动 Idle、Suspicious、Investigate、Search、Chase 行为、导航速度和捕获检测。
 *
 * 关联文件：LRGuardAIController.h；所属领域：AI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "AI/LRGuardAIController.h"

#include "AI/LRAlertComponent.h"
#include "AI/LRGuardCharacter.h"
#include "AI/LRGuardPerceptionRules.h"
#include "Components/ActorComponent.h"
#include "Components/StateTreeAIComponent.h"
#include "Core/LRGameplayTags.h"
#include "Core/LRLog.h"
#include "Data/LRGameTuningSet.h"
#include "Data/LRGuardDefinition.h"
#include "Data/LRGuardTuning.h"
#include "DrawDebugHelpers.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Stealth/LRGuardVisibility.h"
#include "StateTree.h"
#include "TimerManager.h"

/**
 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
 */
ALRGuardAIController::ALRGuardAIController()
{
	PrimaryActorTick.bCanEverTick = false;
	bStartAILogicOnPossess = true;
	bStopAILogicOnUnposses = true;
	bAttachToPawn = true;
	StateTreeAI = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAI"));
	StateTreeAI->SetStartLogicAutomatically(false);
	AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
	SetPerceptionComponent(*AIPerception);
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	ConfigurePerception();
	AIPerception->ConfigureSense(*SightConfig);
	AIPerception->ConfigureSense(*HearingConfig);
	AIPerception->SetDominantSense(SightConfig->GetSenseImplementation());
}

/**
 * @brief 在进入世界后解析运行时依赖、绑定事件并启动所需计时器；构造阶段不访问 World 或玩家对象。
 */
void ALRGuardAIController::BeginPlay()
{
	Super::BeginPlay();
	const UGameInstance* gameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const ULRGameInstanceSubsystem* subsystem = gameInstance ? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	Tuning = subsystem && subsystem->GetTuningSet() ? subsystem->GetTuningSet()->Guard : nullptr;
	if (!ensureMsgf(Tuning, TEXT("%s requires Guard tuning."), *GetNameSafe(this)))
	{
		return;
	}
	ConfigurePerception();
	AIPerception->RequestStimuliListenerUpdate();
	AIPerception->OnTargetPerceptionUpdated.AddDynamic(this, &ALRGuardAIController::HandlePerception);
	GetWorld()->GetTimerManager().SetTimer(CaptureTimer, this, &ALRGuardAIController::HandleCaptureTimer,
		Tuning->CaptureCheckIntervalSeconds, true);
}

/**
 * @brief 解除委托并清理计时器或缓存，避免关卡切换和对象销毁后继续收到回调。
 * @param endPlayReason Unreal 提供的结束原因，用于区分销毁、关卡切换和退出。
 */
void ALRGuardAIController::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	if (AIPerception)
	{
		AIPerception->OnTargetPerceptionUpdated.RemoveDynamic(this, &ALRGuardAIController::HandlePerception);
	}
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(CaptureTimer);
		GetWorld()->GetTimerManager().ClearTimer(SearchTimer);
	}
	Super::EndPlay(endPlayReason);
}

/**
 * @brief 处理 On Possess 事件，将引擎回调转换为对应领域状态更新。
 * @param inPawn Controller 新接管的 Pawn；期望为 ALRGuardCharacter。
 */
void ALRGuardAIController::OnPossess(APawn* inPawn)
{
	ALRGuardCharacter* guard = Cast<ALRGuardCharacter>(inPawn);
	const ULRGuardDefinition* definition = guard ? guard->GetDefinition() : nullptr;
	if (definition)
	{
		StateTreeAI->SetStateTree(definition->Behavior.LoadSynchronous());
	}
	Super::OnPossess(inPawn);
	Alert = guard ? guard->GetAlertComponent() : nullptr;
	if (Alert.IsValid())
	{
		Alert->OnAlertChanged.AddDynamic(this, &ALRGuardAIController::HandleAlertChanged);
		EnterBehavior(Alert->GetBehaviorState());
	}
}

/**
 * @brief 处理 On Un Possess 事件，将引擎回调转换为对应领域状态更新。
 */
void ALRGuardAIController::OnUnPossess()
{
	if (Alert.IsValid())
	{
		Alert->OnAlertChanged.RemoveDynamic(this, &ALRGuardAIController::HandleAlertChanged);
	}
	Alert.Reset();
	Super::OnUnPossess();
}

/**
 * @brief 进入指定守卫行为，设置移动速度、焦点、导航目标或搜索超时。
 * @param behavior 要进入或退出的守卫 StateTree 行为状态。
 */
void ALRGuardAIController::EnterBehavior(const ELRGuardBehaviorState behavior)
{
	ActiveBehavior = behavior;
	ALRGuardCharacter* guard = Cast<ALRGuardCharacter>(GetPawn());
	if (!guard || !Alert.IsValid())
	{
		return;
	}
	GetWorld()->GetTimerManager().ClearTimer(SearchTimer);
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
		StopMovement();
		SetFocalPoint(Alert->GetLastDisturbanceLocation());
		GetWorld()->GetTimerManager().SetTimer(SearchTimer, this, &ALRGuardAIController::HandleSearchTimeout,
			GetEffectiveTuning().SearchDurationSeconds, false);
	}
	else if (behavior == ELRGuardBehaviorState::Suspicious)
	{
		StopMovement();
		SetFocalPoint(Alert->GetLastDisturbanceLocation());
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
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(SearchTimer);
	}
	StopMovement();
	ClearFocus(EAIFocusPriority::Gameplay);
}

/**
 * @brief 处理 On Move Completed 事件，将引擎回调转换为对应领域状态更新。
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
 * @brief 把 UE 感知刺激转换为可见/听见事件、异常位置和警戒原因标签。
 * @param actor 本次查询、交互或事件涉及的 Actor。
 * @param stimulus 时间值 `stimulus`，单位为秒。
 */
void ALRGuardAIController::HandlePerception(AActor* actor, const FAIStimulus stimulus)
{
	if (!actor || !Alert.IsValid())
	{
		return;
	}
	if (stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
	{
		const bool bVisible = stimulus.WasSuccessfullySensed() && CanConfirmSight(actor);
		Alert->SetSightTarget(actor, bVisible, stimulus.StimulusLocation);
	}
	else if (stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>() && stimulus.WasSuccessfullySensed())
	{
		FGameplayTag reason = FGameplayTag::RequestGameplayTag(stimulus.Tag, false);
		if (!reason.IsValid())
		{
			reason = LRGameplayTags::NoiseInteraction;
		}
		Alert->ApplyAlertDelta(GetEffectiveTuning().HearingAlertAmount, stimulus.StimulusLocation, actor, reason);
	}
}

/**
 * @brief 处理 Handle Alert Changed 事件，将引擎回调转换为对应领域状态更新。
 * @param previousLevel 本次操作使用的计数、增量或索引 `previousLevel`；由函数校验合法范围。
 * @param currentLevel 本次操作使用的计数、增量或索引 `currentLevel`；由函数校验合法范围。
 * @param currentState 本次操作使用的 `currentState` 枚举或模式值。
 * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
 * @param disturbanceLocation 空间值 `disturbanceLocation`；距离和位置使用 Unreal 厘米单位。
 */
void ALRGuardAIController::HandleAlertChanged(const int32 previousLevel, const int32 currentLevel,
	const ELRGuardBehaviorState currentState, const FGameplayTag reason, const FVector disturbanceLocation)
{
	StateTreeAI->SendStateTreeEvent(LRGameplayTags::AIEventAlertChanged, FConstStructView(), reason.GetTagName());
	if (!StateTreeAI->IsRunning())
	{
		EnterBehavior(currentState);
	}
}

/**
 * @brief 用 Guard 调优资产配置 UE Sight/Hearing 感知，包括完整视野角换算、距离和阵营检测。
 */
void ALRGuardAIController::ConfigurePerception()
{
	const ULRGuardTuning& tuning = GetEffectiveTuning();
	SightConfig->SightRadius = tuning.SightRadius;
	SightConfig->LoseSightRadius = tuning.LoseSightRadius;
	SightConfig->PeripheralVisionAngleDegrees = tuning.SightConeDegrees * 0.5f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	HearingConfig->HearingRange = tuning.MaxHearingRange * tuning.HearingRangeMultiplier;
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
}

/**
 * @brief 按可调低频计时检查追逐目标距离；进入捕获半径后触发玩家死亡与 Memory 流程。
 */
void ALRGuardAIController::HandleCaptureTimer()
{
	if (!Alert.IsValid() || Alert->GetBehaviorState() != ELRGuardBehaviorState::Chase)
	{
		return;
	}
	AActor* target = Alert->GetTargetActor();
	if (!CanConfirmSight(target))
	{
		Alert->SetSightTarget(target, false, target ? target->GetActorLocation() : FVector::ZeroVector);
		return;
	}
	ALRGuardCharacter* guard = Cast<ALRGuardCharacter>(GetPawn());
	if (guard && FVector::Dist2D(guard->GetActorLocation(), target->GetActorLocation()) <= GetEffectiveTuning().CaptureRadius)
	{
		guard->CaptureTarget(target);
	}
}

/**
 * @brief 处理 Handle Search Timeout 事件，将引擎回调转换为对应领域状态更新。
 */
void ALRGuardAIController::HandleSearchTimeout()
{
	if (Alert.IsValid())
	{
		Alert->ResetAfterSearch();
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
 * @brief 判断 Can Confirm Sight 对应条件；不产生玩法副作用。
 * @param actor 本次查询、交互或事件涉及的 Actor。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ALRGuardAIController::CanConfirmSight(AActor* actor) const
{
	const APawn* guardPawn = GetPawn();
	if (!actor || !guardPawn)
	{
		return false;
	}
	const FVector toTarget = actor->GetActorLocation() - guardPawn->GetActorLocation();
	const float distance = toTarget.Size2D();
	const float forwardDot = FVector::DotProduct(guardPawn->GetActorForwardVector().GetSafeNormal2D(),
		toTarget.GetSafeNormal2D());
	return LRGuardPerceptionRules::CanConfirmSight(distance, forwardDot, !LineOfSightTo(actor),
		IsHiddenFromGuard(actor), GetEffectiveTuning());
}

/**
 * @brief 判断 Is Hidden From Guard 对应条件；不产生玩法副作用。
 * @param actor 本次查询、交互或事件涉及的 Actor。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ALRGuardAIController::IsHiddenFromGuard(AActor* actor) const
{
	if (actor->GetClass()->ImplementsInterface(ULRGuardVisibility::StaticClass()))
	{
		return !ILRGuardVisibility::Execute_IsVisibleToGuard(actor, const_cast<ALRGuardAIController*>(this));
	}
	for (UActorComponent* component : actor->GetComponents())
	{
		if (component && component->GetClass()->ImplementsInterface(ULRGuardVisibility::StaticClass())
			&& !ILRGuardVisibility::Execute_IsVisibleToGuard(component, const_cast<ALRGuardAIController*>(this)))
		{
			return true;
		}
	}
	return false;
}

/**
 * @brief 输出守卫行为、警戒值和最后异常点，并按调试开关绘制视野与听觉范围。
 */
void ALRGuardAIController::LogAndDrawDiagnostics() const
{
	const APawn* guard = GetPawn();
	if (!guard || !Alert.IsValid())
	{
		return;
	}
	const ULRGuardTuning& tuning = GetEffectiveTuning();
	UE_LOG(LogLostRunicAI, Display, TEXT("Guard=%s Alert=%d State=%d Target=%s Reason=%s Location=%s"),
		*GetNameSafe(guard), Alert->GetAlertLevel(), static_cast<int32>(Alert->GetBehaviorState()),
		*GetNameSafe(Alert->GetTargetActor()), *Alert->GetLastReason().ToString(),
		*Alert->GetLastDisturbanceLocation().ToCompactString());
	const FVector origin = guard->GetActorLocation();
	DrawDebugCone(GetWorld(), origin, guard->GetActorForwardVector(), tuning.SightRadius,
		FMath::DegreesToRadians(tuning.SightConeDegrees * 0.5f), FMath::DegreesToRadians(tuning.SightConeDegrees * 0.5f),
		16, FColor::Yellow, false, 5.0f);
	DrawDebugSphere(GetWorld(), origin, tuning.MaxHearingRange, 32, FColor::Cyan, false, 5.0f);
	DrawDebugSphere(GetWorld(), origin, tuning.CaptureRadius, 16, FColor::Red, false, 5.0f);
}

/**
 * @brief 查询 Effective Tuning；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
const ULRGuardTuning& ALRGuardAIController::GetEffectiveTuning() const
{
	return Tuning ? *Tuning : *GetDefault<ULRGuardTuning>();
}
