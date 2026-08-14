/**
 * @file LRGuardAIController.cpp
 * @brief 守卫控制器生命周期：构造、BeginPlay/EndPlay、OnPossess/OnUnPossess、调优解析与 StateTree 启动接线。感知与行为实现分别位于 LRGuardAIControllerPerception.cpp / LRGuardAIControllerBehavior.cpp。
 *
 * 关联文件：LRGuardAIController.h；所属领域：AI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "AI/LRGuardAIController.h"

#include "AI/LRAlertComponent.h"
#include "AI/LRAlertRules.h"
#include "AI/LRGuardCharacter.h"
#include "AI/LRGuardPerceptionRules.h"
#include "Components/StateTreeAIComponent.h"
#include "Core/LRLog.h"
#include "Data/LRGameTuningSet.h"
#include "Data/LRGuardDefinition.h"
#include "Data/LRGuardTuning.h"
#include "Data/LRStateTuning.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "Items/LRCourageResponseComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
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
	// StateTree 由 OnPossess 依次完成定义解析、引用校验、SetStateTree、StartLogic，禁用自动启动。
	StateTreeAI->SetStartLogicAutomatically(false);
	AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
	SetPerceptionComponent(*AIPerception);
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
}

/**
 * @brief 在进入世界后解析运行时依赖、绑定事件并启动所需计时器；构造阶段不访问 World 或玩家对象。
 */
void ALRGuardAIController::BeginPlay()
{
	Super::BeginPlay();
	const UGameInstance* gameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const ULRGameInstanceSubsystem* subsystem = gameInstance ? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	if (subsystem && subsystem->GetTuningSet())
	{
		Tuning = subsystem->GetTuningSet()->Guard;
		StateTuning = subsystem->GetTuningSet()->State;
	}
	if (!ensureMsgf(Tuning && StateTuning, TEXT("%s requires Guard and State tuning."), *GetNameSafe(this)))
	{
		return;
	}
	ConfigurePerception();
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
		GetWorld()->GetTimerManager().ClearTimer(StunTimer);
	}
	Super::EndPlay(endPlayReason);
}

/**
 * @brief 处理 On Possess 事件：解析定义并校验引用、SetStateTree 后 StartLogic，绑定警戒与击退事件。
 * @param inPawn Controller 新接管的 Pawn；期望为 ALRGuardCharacter。
 */
void ALRGuardAIController::OnPossess(APawn* inPawn)
{
	Super::OnPossess(inPawn);
	ALRGuardCharacter* guard = Cast<ALRGuardCharacter>(inPawn);
	Alert = guard ? guard->GetAlertComponent() : nullptr;
	if (Alert.IsValid())
	{
		Alert->OnAlertChanged.AddDynamic(this, &ALRGuardAIController::HandleAlertChanged);
	}
	if (guard)
	{
		if (ULRCourageResponseComponent* courage = guard->GetCourageResponseComponent())
		{
			courage->OnKnockbackApplied.AddDynamic(this, &ALRGuardAIController::HandleKnockback);
		}
		ULRGuardDefinition* definition = guard->GetDefinition();
		if (definition && definition->Behavior)
		{
			StateTreeAI->SetStateTree(definition->Behavior);
			if (!StateTreeAI->IsRunning())
			{
				StateTreeAI->StartLogic();
			}
		}
		else
		{
			UE_LOG(LogLostRunicAI, Warning, TEXT("Guard=%s definition or Behavior StateTree is missing; using controller fallback."),
				*GetNameSafe(guard));
		}
	}
}

/**
 * @brief 处理 On Un Possess 事件：解绑警戒与击退委托，停止 StateTree 逻辑。
 */
void ALRGuardAIController::OnUnPossess()
{
	if (Alert.IsValid())
	{
		Alert->OnAlertChanged.RemoveDynamic(this, &ALRGuardAIController::HandleAlertChanged);
	}
	if (ALRGuardCharacter* guard = Cast<ALRGuardCharacter>(GetPawn()))
	{
		if (ULRCourageResponseComponent* courage = guard->GetCourageResponseComponent())
		{
			courage->OnKnockbackApplied.RemoveDynamic(this, &ALRGuardAIController::HandleKnockback);
		}
	}
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(StunTimer);
	}
	if (StateTreeAI->IsRunning())
	{
		StateTreeAI->StopLogic(TEXT("OnUnPossess"));
	}
	Alert.Reset();
	Super::OnUnPossess();
}

/**
 * @brief 查询 Resolved Behavior；行为状态唯一权威解析（眩晕优先，否则警戒推导），StateTree 只执行该结果。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
ELRGuardBehaviorState ALRGuardAIController::GetResolvedBehavior() const
{
	return LRAlertRules::ResolveTargetBehavior(bStunned, Alert.IsValid() ? Alert->GetAlertLevel() : 0,
		Alert.IsValid() && Alert->HasConfirmedSight(), Alert.IsValid() && Alert->IsSearching());
}

/**
 * @brief 查询 Effective Tuning；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
const ULRGuardTuning& ALRGuardAIController::GetEffectiveTuning() const
{
	return Tuning ? *Tuning : *GetDefault<ULRGuardTuning>();
}
