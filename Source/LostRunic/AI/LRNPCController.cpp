/**
 * @file LRNPCController.cpp
 * @brief 通用 NPC 控制器实现：Hearing 感知、StateTree 生命周期、巡逻、低频玩家朝向与限时噪声反应。
 *
 * 关联文件：LRNPCController.h；所属领域：AI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "AI/LRNPCController.h"

#include "AI/LRNPCCharacter.h"
#include "Components/StateTreeAIComponent.h"
#include "Core/LRGameplayTags.h"
#include "Core/LRLog.h"
#include "Data/LRGameTuningSet.h"
#include "Data/LRNPCDefinition.h"
#include "Data/LRNPCTuning.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "TimerManager.h"

/**
 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
 */
ALRNPCController::ALRNPCController()
{
	PrimaryActorTick.bCanEverTick = false;
	bStartAILogicOnPossess = true;
	bStopAILogicOnUnposses = true;
	bAttachToPawn = true;
	StateTreeAI = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAI"));
	StateTreeAI->SetStartLogicAutomatically(false);
	AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
	SetPerceptionComponent(*AIPerception);
	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
}

/**
 * @brief 在进入世界后解析运行时依赖、绑定事件并启动所需计时器；构造阶段不访问 World 或玩家对象。
 */
void ALRNPCController::BeginPlay()
{
	Super::BeginPlay();
	const UGameInstance* gameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const ULRGameInstanceSubsystem* subsystem = gameInstance ? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	Tuning = subsystem && subsystem->GetTuningSet() ? subsystem->GetTuningSet()->NPC : nullptr;
	if (!ensureMsgf(Tuning, TEXT("%s requires NPC tuning."), *GetNameSafe(this)))
	{
		return;
	}
	HearingConfig->HearingRange = 5000.0f;
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	AIPerception->ConfigureSense(*HearingConfig);
	AIPerception->SetDominantSense(HearingConfig->GetSenseImplementation());
	AIPerception->OnTargetPerceptionUpdated.AddDynamic(this, &ALRNPCController::HandlePerception);
}

/**
 * @brief 解除委托并清理计时器或缓存，避免关卡切换和对象销毁后继续收到回调。
 * @param endPlayReason Unreal 提供的结束原因，用于区分销毁、关卡切换和退出。
 */
void ALRNPCController::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	if (AIPerception)
	{
		AIPerception->OnTargetPerceptionUpdated.RemoveDynamic(this, &ALRNPCController::HandlePerception);
	}
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(LookAtTimer);
		GetWorld()->GetTimerManager().ClearTimer(ReactionTimer);
	}
	Super::EndPlay(endPlayReason);
}

/**
 * @brief 处理 On Possess 事件：解析定义、SetStateTree 后 StartLogic，并绑定 Hearing 感知。
 * @param inPawn Controller 新接管的 Pawn；期望为 ALRNPCCharacter。
 */
void ALRNPCController::OnPossess(APawn* inPawn)
{
	Super::OnPossess(inPawn);
	Npc = Cast<ALRNPCCharacter>(inPawn);
	Definition = Npc.IsValid() ? Npc->GetDefinition() : nullptr;
	if (Definition.IsValid() && Definition->Behavior)
	{
		StateTreeAI->SetStateTree(Definition->Behavior);
		if (!StateTreeAI->IsRunning())
		{
			StateTreeAI->StartLogic();
		}
	}
	else
	{
		UE_LOG(LogLostRunicAI, Warning, TEXT("NPC=%s definition or Behavior StateTree is missing; using controller fallback."),
			*GetNameSafe(inPawn));
	}
}

/**
 * @brief 处理 On Un Possess 事件：解绑感知并停止 StateTree 逻辑。
 */
void ALRNPCController::OnUnPossess()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(LookAtTimer);
		GetWorld()->GetTimerManager().ClearTimer(ReactionTimer);
	}
	if (StateTreeAI->IsRunning())
	{
		StateTreeAI->StopLogic(TEXT("OnUnPossess"));
	}
	Npc.Reset();
	Definition.Reset();
	Super::OnUnPossess();
}

/**
 * @brief 处理 On Move Completed 事件：巡逻点到达续走下一段。
 * @param requestId 稳定标识 `requestId`；用于内容查询和存档，不依赖显示名或数组序号。
 * @param result 本次领域操作的结构化数据 `result`；字段语义由对应 USTRUCT 定义。
 */
void ALRNPCController::OnMoveCompleted(const FAIRequestID requestId, const FPathFollowingResult& result)
{
	Super::OnMoveCompleted(requestId, result);
	if (!result.IsSuccess())
	{
		return;
	}
	if (ActiveBehavior == ELRNPCBehaviorState::Patrol)
	{
		++PatrolIndex;
		StartPatrolMove();
	}
}

/**
 * @brief 进入指定 NPC 行为：Idle 启动玩家朝向检测、Patrol 巡逻、ReactToNoise 转向声源限时反应、Conversation 停止一切反应。
 * @param behavior 要进入或退出的 NPC StateTree 行为状态。
 */
void ALRNPCController::EnterBehavior(const ELRNPCBehaviorState behavior)
{
	ActiveBehavior = behavior;
	ALRNPCCharacter* npc = Npc.Get();
	if (!npc)
	{
		return;
	}
	switch (behavior)
	{
	case ELRNPCBehaviorState::Idle:
		StopMovement();
		ClearFocus(EAIFocusPriority::Gameplay);
		StartLookAtTimer();
		break;
	case ELRNPCBehaviorState::Patrol:
		StopLookAtTimer();
		npc->GetCharacterMovement()->MaxWalkSpeed = GetEffectiveTuning().PatrolSpeedCm;
		StartPatrolMove();
		break;
	case ELRNPCBehaviorState::ReactToNoise:
		StopLookAtTimer();
		StopMovement();
		SetFocalPoint(LastNoiseLocation);
		StartNoiseReaction(LastNoiseLocation);
		break;
	case ELRNPCBehaviorState::Conversation:
		StopLookAtTimer();
		StopNoiseReaction();
		StopMovement();
		ClearFocus(EAIFocusPriority::Gameplay);
		break;
	}
}

/**
 * @brief 退出指定 NPC 行为并清理该状态拥有的导航、焦点或计时器。
 * @param behavior 要进入或退出的 NPC StateTree 行为状态。
 */
void ALRNPCController::ExitBehavior(const ELRNPCBehaviorState behavior)
{
	StopLookAtTimer();
	StopNoiseReaction();
	StopMovement();
	ClearFocus(EAIFocusPriority::Gameplay);
}

/**
 * @brief 启动/停止 Idle 低频玩家朝向检测（任务节点调用）。
 */
void ALRNPCController::StartLookAtTimer()
{
	if (!GetWorld() || GetWorld()->GetTimerManager().IsTimerActive(LookAtTimer))
	{
		return;
	}
	GetWorld()->GetTimerManager().SetTimer(LookAtTimer, this, &ALRNPCController::HandleLookAtTimer,
		GetEffectiveTuning().LookAtIntervalSeconds, true);
}

/**
 * @brief 停止 Idle 低频玩家朝向检测（任务节点调用）。
 */
void ALRNPCController::StopLookAtTimer()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(LookAtTimer);
	}
}

/**
 * @brief 开始限时噪声反应：转向声源并按 NoiseReactionDurationSeconds 计时，结束后发送 NPCReactionEnded 事件。
 * @param location 世界空间位置，Unreal 单位为厘米。
 */
void ALRNPCController::StartNoiseReaction(const FVector location)
{
	LastNoiseLocation = location;
	if (!GetWorld())
	{
		return;
	}
	GetWorld()->GetTimerManager().ClearTimer(ReactionTimer);
	GetWorld()->GetTimerManager().SetTimer(ReactionTimer, this, &ALRNPCController::HandleReactionTimeout,
		GetEffectiveTuning().NoiseReactionDurationSeconds, false);
}

/**
 * @brief 结束噪声反应计时（任务退出时调用）。
 */
void ALRNPCController::StopNoiseReaction()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(ReactionTimer);
	}
}

/**
 * @brief 对话开始：进入 Conversation（高优先级），停止一切反应。
 */
void ALRNPCController::NotifyDialogueStarted()
{
	DispatchBehaviorEvent(LRGameplayTags::AIEventNPCDialogueStarted, ELRNPCBehaviorState::Conversation);
}

/**
 * @brief 对话结束：回到配置的默认行为。
 */
void ALRNPCController::NotifyDialogueEnded()
{
	DispatchBehaviorEvent(LRGameplayTags::AIEventNPCDialogueEnded, GetBaseBehavior());
}

/**
 * @brief 把 UE 听觉刺激转换为噪声反应；Conversation 期间只触发表现钩子，不切换行为。
 * @param actor 本次查询、交互或事件涉及的 Actor。
 * @param stimulus 时间值 `stimulus`，单位为秒。
 */
void ALRNPCController::HandlePerception(AActor* actor, const FAIStimulus stimulus)
{
	if (!actor || !Npc.IsValid() || stimulus.Type != UAISense::GetSenseID<UAISense_Hearing>()
		|| !stimulus.WasSuccessfullySensed())
	{
		return;
	}
	FGameplayTag reason = FGameplayTag::RequestGameplayTag(stimulus.Tag, false);
	if (!reason.IsValid())
	{
		reason = LRGameplayTags::NoiseInteraction;
	}
	LastNoiseLocation = stimulus.StimulusLocation;
	Npc->NotifyNoiseHeard(stimulus.StimulusLocation, reason);
	// Conversation 为高优先级行为：普通噪声不打断对话（表现钩子已触发）。
	if (ActiveBehavior == ELRNPCBehaviorState::Conversation)
	{
		return;
	}
	DispatchBehaviorEvent(LRGameplayTags::AIEventNPCNoiseHeard, ELRNPCBehaviorState::ReactToNoise);
}

/**
 * @brief 处理 Handle Look At Timer 事件：距离、视线与可达性满足时朝向玩家。
 */
void ALRNPCController::HandleLookAtTimer()
{
	ALRNPCCharacter* npc = Npc.Get();
	if (!npc || !GetWorld())
	{
		return;
	}
	const APawn* playerPawn = GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr;
	if (!playerPawn)
	{
		return;
	}
	const float distance = FVector::Dist2D(npc->GetActorLocation(), playerPawn->GetActorLocation());
	if (distance <= GetEffectiveTuning().LookAtPlayerRadiusCm && LineOfSightTo(playerPawn))
	{
		const FVector toPlayer = playerPawn->GetActorLocation() - npc->GetActorLocation();
		npc->SetActorRotation(FRotator(0.0f, toPlayer.Rotation().Yaw, 0.0f));
	}
}

/**
 * @brief 处理 Handle Reaction Timeout 事件：噪声反应结束，发送 NPCReactionEnded。
 */
void ALRNPCController::HandleReactionTimeout()
{
	if (ActiveBehavior != ELRNPCBehaviorState::ReactToNoise)
	{
		return;
	}
	DispatchBehaviorEvent(LRGameplayTags::AIEventNPCReactionEnded, GetBaseBehavior());
}

/**
 * @brief 开始 Start Patrol Move 流程，建立本次操作拥有的状态、委托或计时器。
 */
void ALRNPCController::StartPatrolMove()
{
	ALRNPCCharacter* npc = Npc.Get();
	if (!npc || npc->GetPatrolPointCount() == 0)
	{
		StopMovement();
		return;
	}
	PatrolIndex %= npc->GetPatrolPointCount();
	MoveToActor(npc->GetPatrolPoint(PatrolIndex), 50.0f);
}

/**
 * @brief 解析配置的默认行为到运行态枚举。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
ELRNPCBehaviorState ALRNPCController::GetBaseBehavior() const
{
	return Definition.IsValid() && Definition->DefaultBehavior == ENPCBaseBehavior::Patrol
		? ELRNPCBehaviorState::Patrol : ELRNPCBehaviorState::Idle;
}

/**
 * @brief 向 StateTree 发送行为事件；树未运行时直接进入行为。
 * @param event 本次领域操作的结构化数据 `event`；字段语义由对应 USTRUCT 定义。
 * @param behavior 要进入或退出的 NPC StateTree 行为状态。
 */
void ALRNPCController::DispatchBehaviorEvent(const FGameplayTag event, const ELRNPCBehaviorState behavior)
{
	if (StateTreeAI->IsRunning())
	{
		StateTreeAI->SendStateTreeEvent(event, FConstStructView(), FName());
	}
	else
	{
		EnterBehavior(behavior);
	}
}

/**
 * @brief 查询 Effective Tuning；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
const ULRNPCTuning& ALRNPCController::GetEffectiveTuning() const
{
	return Tuning ? *Tuning : *GetDefault<ULRNPCTuning>();
}
