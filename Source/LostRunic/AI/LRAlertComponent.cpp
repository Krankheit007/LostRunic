/**
 * @file LRAlertComponent.cpp
 * @brief 保存单个守卫 0-11 警戒值、最后异常位置、目标和观察计时；所有升降都携带 Gameplay Tag 原因并广播事件。
 *
 * 关联文件：LRAlertComponent.h；所属领域：AI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "AI/LRAlertComponent.h"

#include "AI/LRAlertRules.h"
#include "Core/LRGameplayTags.h"
#include "Core/LRLog.h"
#include "Data/LRGameTuningSet.h"
#include "Data/LRGuardTuning.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "TimerManager.h"

/**
 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
 */
ULRAlertComponent::ULRAlertComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

/**
 * @brief 在进入世界后解析运行时依赖、绑定事件并启动所需计时器；构造阶段不访问 World 或玩家对象。
 */
void ULRAlertComponent::BeginPlay()
{
	Super::BeginPlay();
	const UGameInstance* gameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const ULRGameInstanceSubsystem* subsystem = gameInstance ? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	Tuning = subsystem && subsystem->GetTuningSet() ? subsystem->GetTuningSet()->Guard : nullptr;
	if (!ensureMsgf(Tuning, TEXT("%s requires Guard tuning."), *GetNameSafe(this)))
	{
		return;
	}
	LastStimulusTimeSeconds = GetWorld()->GetTimeSeconds();
	GetWorld()->GetTimerManager().SetTimer(DecayTimer, this, &ULRAlertComponent::HandleDecayTimer,
		Tuning->AlertDecayIntervalSeconds, true);
}

/**
 * @brief 解除委托并清理计时器或缓存，避免关卡切换和对象销毁后继续收到回调。
 * @param endPlayReason Unreal 提供的结束原因，用于区分销毁、关卡切换和退出。
 */
void ULRAlertComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(DecayTimer);
		GetWorld()->GetTimerManager().ClearTimer(ObservationTimer);
	}
	Super::EndPlay(endPlayReason);
}

/**
 * @brief 把警戒增减限制在 0-11，并记录原因、异常位置与目标后广播变化；警戒归零时清理目标与观察状态。
 * @param delta 调用方提供的 `delta`，只在本次操作范围内使用。
 * @param location 世界空间位置，Unreal 单位为厘米。
 * @param target 本次规则检查或操作的目标对象。
 * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
 */
void ULRAlertComponent::ApplyAlertDelta(const int32 delta, const FVector location, AActor* target,
	const FGameplayTag reason)
{
	const int32 previousLevel = AlertLevel;
	const ELRGuardBehaviorState previousState = GetBehaviorState();
	AlertLevel = LRAlertRules::ApplyDelta(AlertLevel, delta);
	LastDisturbanceLocation = location;
	if (target)
	{
		TargetActor = target;
	}
	if (delta > 0)
	{
		bSearching = false;
	}
	LastStimulusTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	if (AlertLevel <= 0)
	{
		ClearWhenAlertZero();
	}
	BroadcastChange(previousLevel, previousState, reason);
}

/**
 * @brief 吸引注意语义入口：按档位冷却门控（CD 内刺激完全忽略，不改变观察状态），每次 +AttractAlertAmount，并重置 3s 观察窗口。
 * @param location 世界空间位置，Unreal 单位为厘米。
 * @param target 本次规则检查或操作的目标对象。
 * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
 */
void ULRAlertComponent::ApplyAttract(const FVector location, AActor* target, const FGameplayTag reason)
{
	const ULRGuardTuning& tuning = GetEffectiveTuning();
	const double now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	// 从 0 起的首次吸引立即生效（设计：0 -> 1）；之后的增加受档位冷却门控，CD 内完全忽略。
	if (AlertLevel > 0
		&& !LRAlertRules::IsIncreaseAllowed(now, LastIncreaseTimeSeconds,
			LRAlertRules::ResolveAttractIncreaseCooldown(AlertLevel, bFirstIncreaseInBand, tuning)))
	{
		UE_LOG(LogLostRunicAI, Verbose, TEXT("Guard=%s attract ignored (cooldown) reason=%s"),
			*GetNameSafe(GetOwner()), *reason.GetTagName().ToString());
		return;
	}

	const int32 previousLevel = AlertLevel;
	const ELRGuardBehaviorState previousState = GetBehaviorState();
	const bool bCrossingIntoBand = AlertLevel < tuning.SightInvestigateLevel;
	AlertLevel = LRAlertRules::ApplyDelta(AlertLevel, tuning.AttractAlertAmount);
	LastDisturbanceLocation = location;
	if (target)
	{
		TargetActor = target;
	}
	bSearching = false;
	LastIncreaseTimeSeconds = now;
	LastStimulusTimeSeconds = now;
	if (bFirstIncreaseInBand)
	{
		bFirstIncreaseInBand = false;
	}
	if (bCrossingIntoBand && AlertLevel >= tuning.SightInvestigateLevel)
	{
		bFirstIncreaseInBand = true;
	}
	StartObservation();
	BroadcastChange(previousLevel, previousState, reason);
}

/**
 * @brief 更新 Sight Target，并在需要时同步组件状态或广播变化事件；按 4.2.1 分档：<6 看见升到 6，6-10 看见升到 11，11 丢失降回 10。
 * @param target 本次规则检查或操作的目标对象。
 * @param bVisible 布尔开关 `bVisible`；true 表示启用或条件成立，false 表示禁用或条件不成立。
 * @param lastKnownLocation 空间值 `lastKnownLocation`；距离和位置使用 Unreal 厘米单位。
 */
void ULRAlertComponent::SetSightTarget(AActor* target, const bool bVisible, const FVector lastKnownLocation)
{
	const int32 previousLevel = AlertLevel;
	const ELRGuardBehaviorState previousState = GetBehaviorState();
	LastDisturbanceLocation = lastKnownLocation;
	TargetActor = target;
	bHasConfirmedSight = bVisible;
	if (bVisible)
	{
		const ULRGuardTuning& tuning = GetEffectiveTuning();
		if (AlertLevel < tuning.SightInvestigateLevel)
		{
			AlertLevel = tuning.SightInvestigateLevel;
			bFirstIncreaseInBand = true;
		}
		else if (AlertLevel < tuning.SightChaseLevel)
		{
			AlertLevel = tuning.SightChaseLevel;
		}
	}
	else if (AlertLevel >= GetEffectiveTuning().SightChaseLevel)
	{
		// 11 丢失视线 -> 10，前往最后看见位置；不置 bSearching，使行为解析自然落到 Investigate。
		AlertLevel = 10;
	}
	LastStimulusTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	BroadcastChange(previousLevel, previousState,
		bVisible ? LRGameplayTags::SightPlayer : LRGameplayTags::SightPlayerLost);
}

/**
 * @brief 标记守卫已到达最后异常位置，使 StateTree 从 Investigate 转入 Search，并开始 3s 抵达观察。
 */
void ULRAlertComponent::MarkInvestigationReached()
{
	const ELRGuardBehaviorState previousState = GetBehaviorState();
	bSearching = AlertLevel > 0;
	StartObservation();
	BroadcastChange(AlertLevel, previousState, LRGameplayTags::SearchReached);
}

/**
 * @brief 搜索超时后清理目标与异常状态，使警戒系统回到可衰减的巡逻阶段。
 */
void ULRAlertComponent::ResetAfterSearch()
{
	const int32 previousLevel = AlertLevel;
	const ELRGuardBehaviorState previousState = GetBehaviorState();
	AlertLevel = 0;
	bSearching = false;
	bHasConfirmedSight = false;
	bObserving = false;
	TargetActor.Reset();
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(ObservationTimer);
	}
	BroadcastChange(previousLevel, previousState, LRGameplayTags::SearchTimeout);
}

/**
 * @brief 查询 Behavior State；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
ELRGuardBehaviorState ULRAlertComponent::GetBehaviorState() const
{
	return LRAlertRules::ResolveState(AlertLevel, bHasConfirmedSight, bSearching);
}

/**
 * @brief 处理 Handle Decay Timer 事件，将引擎回调转换为对应领域状态更新。
 */
void ULRAlertComponent::HandleDecayTimer()
{
	if (AlertLevel <= 0 || !GetWorld())
	{
		return;
	}
	if (LRAlertRules::ShouldDecay(bObserving, bHasConfirmedSight, GetBehaviorState()))
	{
		ApplyAlertDelta(-GetEffectiveTuning().AlertDecayAmount, LastDisturbanceLocation,
			TargetActor.Get(), LRGameplayTags::SearchAlertDecay);
	}
}

/**
 * @brief 处理 Handle Observation End 事件，将引擎回调转换为对应领域状态更新；观察结束前警戒维持不动。
 */
void ULRAlertComponent::HandleObservationEnd()
{
	bObserving = false;
}

/**
 * @brief 开始 3 秒观察窗口；观察期间衰减被门控。
 */
void ULRAlertComponent::StartObservation()
{
	if (AlertLevel <= 0 || !GetWorld())
	{
		return;
	}
	bObserving = true;
	GetWorld()->GetTimerManager().ClearTimer(ObservationTimer);
	GetWorld()->GetTimerManager().SetTimer(ObservationTimer, this, &ULRAlertComponent::HandleObservationEnd,
		GetEffectiveTuning().InitialObserveSeconds, false);
}

/**
 * @brief 警戒归零时清理目标、搜索与观察状态；不额外广播（归零变化本身已广播）。
 */
void ULRAlertComponent::ClearWhenAlertZero()
{
	bSearching = false;
	bHasConfirmedSight = false;
	bObserving = false;
	TargetActor.Reset();
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(ObservationTimer);
	}
}

/**
 * @brief 查询当前只读警戒快照（等级、归一化进度、显示档位、行为与满值标志）；UI 绑定 OnAlertSnapshotChanged 后应立即读取本值，避免首帧不同步。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FLRAlertSnapshot ULRAlertComponent::GetAlertSnapshot() const
{
	FLRAlertSnapshot snapshot;
	snapshot.Level = AlertLevel;
	snapshot.Fraction = AlertLevel / static_cast<float>(LRAlertRules::MaxAlertLevel);
	snapshot.Tier = LRAlertRules::ResolveAlertTier(AlertLevel);
	snapshot.Behavior = GetBehaviorState();
	snapshot.bFullAlert = AlertLevel >= LRAlertRules::MaxAlertLevel;
	return snapshot;
}

/**
 * @brief 广播警戒旧值、新值和原因标签，供 StateTree、UI、日志与测试订阅。
 * @param previousLevel 本次操作使用的计数、增量或索引 `previousLevel`；由函数校验合法范围。
 * @param previousState 本次操作使用的 `previousState` 枚举或模式值。
 * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
 */
void ULRAlertComponent::BroadcastChange(const int32 previousLevel, const ELRGuardBehaviorState previousState,
	const FGameplayTag reason)
{
	LastReason = reason;
	const ELRGuardBehaviorState currentState = GetBehaviorState();
	UE_LOG(LogLostRunicAI, Display, TEXT("Guard=%s alert %d -> %d state %d -> %d reason=%s location=%s"),
		*GetNameSafe(GetOwner()), previousLevel, AlertLevel, static_cast<int32>(previousState),
		static_cast<int32>(currentState), *reason.GetTagName().ToString(), *LastDisturbanceLocation.ToCompactString());
	OnAlertChanged.Broadcast(previousLevel, AlertLevel, currentState, reason, LastDisturbanceLocation);
	OnAlertSnapshotChanged.Broadcast(GetAlertSnapshot());
}

/**
 * @brief 查询 Effective Tuning；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
const ULRGuardTuning& ULRAlertComponent::GetEffectiveTuning() const
{
	return Tuning ? *Tuning : *GetDefault<ULRGuardTuning>();
}
