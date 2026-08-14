/**
 * @file LRAlertComponent.h
 * @brief 保存单个守卫 0-11 警戒值、最后异常位置、目标和观察计时；所有升降都携带 Gameplay Tag 原因并广播事件。
 *
 * 关联文件：LRAlertComponent.cpp；所属领域：AI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "AI/LRGuardTypes.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"

#include "LRAlertComponent.generated.h"

class ULRGuardTuning;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FLRAlertChanged, int32, previousLevel, int32, currentLevel,
	ELRGuardBehaviorState, currentState, FGameplayTag, reason, FVector, disturbanceLocation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRAlertSnapshotChanged, const FLRAlertSnapshot&, snapshot);

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(ClassGroup = "Lost Runic", BlueprintType, meta = (BlueprintSpawnableComponent, DisplayName = "Lost Runic Alert"))
class LOSTRUNIC_API ULRAlertComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 */
	ULRAlertComponent();

	/**
	 * @brief 在进入世界后解析运行时依赖、绑定事件并启动所需计时器；构造阶段不访问 World 或玩家对象。
	 */
	virtual void BeginPlay() override;
	/**
	 * @brief 解除委托并清理计时器或缓存，避免关卡切换和对象销毁后继续收到回调。
	 * @param endPlayReason Unreal 提供的结束原因，用于区分销毁、关卡切换和退出。
	 */
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

	/**
	 * @brief 把警戒增减限制在 0-11，并记录原因、异常位置与目标后广播变化；警戒归零时清理目标与观察状态。
	 * @param delta 调用方提供的 `delta`，只在本次操作范围内使用。
	 * @param location 世界空间位置，Unreal 单位为厘米。
	 * @param target 本次规则检查或操作的目标对象。
	 * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|AI|Alert")
	void ApplyAlertDelta(int32 delta, FVector location, AActor* target, FGameplayTag reason);

	/**
	 * @brief 吸引注意语义入口：按档位冷却门控（CD 内刺激完全忽略，不改变观察状态），每次 +AttractAlertAmount，并重置 3s 观察窗口。
	 * @param location 世界空间位置，Unreal 单位为厘米。
	 * @param target 本次规则检查或操作的目标对象。
	 * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|AI|Alert")
	void ApplyAttract(FVector location, AActor* target, FGameplayTag reason);

	/**
	 * @brief 更新 Sight Target，并在需要时同步组件状态或广播变化事件。
	 * @param target 本次规则检查或操作的目标对象。
	 * @param bVisible 布尔开关 `bVisible`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 * @param lastKnownLocation 空间值 `lastKnownLocation`；距离和位置使用 Unreal 厘米单位。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|AI|Alert")
	void SetSightTarget(AActor* target, bool bVisible, FVector lastKnownLocation);

	/**
	 * @brief 标记守卫已到达最后异常位置，使 StateTree 从 Investigate 转入 Search。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|AI|Alert")
	void MarkInvestigationReached();

	/**
	 * @brief 搜索超时后清理目标与异常状态，使警戒系统回到可衰减的巡逻阶段。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|AI|Alert")
	void ResetAfterSearch();

	/**
	 * @brief 查询 Alert Level；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Alert")
	int32 GetAlertLevel() const { return AlertLevel; }

	/**
	 * @brief 查询 Behavior State；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Alert")
	ELRGuardBehaviorState GetBehaviorState() const;

	/**
	 * @brief 查询 Last Disturbance Location；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Alert")
	FVector GetLastDisturbanceLocation() const { return LastDisturbanceLocation; }

	/**
	 * @brief 查询 Target Actor；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Alert")
	AActor* GetTargetActor() const { return TargetActor.Get(); }

	/**
	 * @brief 查询 Last Reason；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Alert")
	FGameplayTag GetLastReason() const { return LastReason; }

	/**
	 * @brief 判断 Has Confirmed Sight 对应条件；不产生玩法副作用。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Alert")
	bool HasConfirmedSight() const { return bHasConfirmedSight; }

	/**
	 * @brief 判断 Is Searching 对应条件；不产生玩法副作用。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Alert")
	bool IsSearching() const { return bSearching; }

	/**
	 * @brief 判断 Is Observing 对应条件；不产生玩法副作用。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Alert")
	bool IsObserving() const { return bObserving; }

	/**
	 * @brief 查询当前只读警戒快照（等级、归一化进度、显示档位、行为与满值标志）；UI 绑定 OnAlertSnapshotChanged 后应立即读取本值，避免首帧不同步。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Alert")
	FLRAlertSnapshot GetAlertSnapshot() const;

	/** 当 Alert Changed 发生时广播；蓝图可绑定该委托以更新表现，不应在回调中改写核心规则。  */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|AI|Alert")
	FLRAlertChanged OnAlertChanged;

	/** 当 Alert Snapshot Changed 发生时广播；世界警戒条 Widget 绑定该只读快照，蓝图只做表现。  */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|AI|Alert")
	FLRAlertSnapshotChanged OnAlertSnapshotChanged;

private:
	/**
	 * @brief 处理 Handle Decay Timer 事件，将引擎回调转换为对应领域状态更新。
	 */
	void HandleDecayTimer();
	/**
	 * @brief 处理 Handle Observation End 事件，将引擎回调转换为对应领域状态更新；观察结束前警戒维持不动。
	 */
	void HandleObservationEnd();
	/**
	 * @brief 开始 3 秒观察窗口；观察期间衰减被门控。
	 */
	void StartObservation();
	/**
	 * @brief 警戒归零时清理目标、搜索与观察状态；不额外广播（归零变化本身已广播）。
	 */
	void ClearWhenAlertZero();
	/**
	 * @brief 广播警戒旧值、新值和原因标签，供 StateTree、UI、日志与测试订阅。
	 * @param previousLevel 本次操作使用的计数、增量或索引 `previousLevel`；由函数校验合法范围。
	 * @param previousState 本次操作使用的 `previousState` 枚举或模式值。
	 * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
	 */
	void BroadcastChange(int32 previousLevel, ELRGuardBehaviorState previousState, FGameplayTag reason);
	/**
	 * @brief 查询 Effective Tuning；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	const ULRGuardTuning& GetEffectiveTuning() const;

	/** Alert Level 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `0`。 仅在蓝图或详情面板中查看，不可编辑。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Alert", meta = (AllowPrivateAccess = "true"))
	int32 AlertLevel = 0;

	/** Last Disturbance Location 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `FVector::ZeroVector`。 仅在蓝图或详情面板中查看，不可编辑。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Alert", meta = (AllowPrivateAccess = "true"))
	FVector LastDisturbanceLocation = FVector::ZeroVector;

	/** 运行时解析出的调优资产缓存；不序列化，不由蓝图编辑。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TObjectPtr<ULRGuardTuning> Tuning;

	/** Target Actor 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> TargetActor;

	/** Last Reason 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	FGameplayTag LastReason;
	/** Last Stimulus Time Seconds 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	double LastStimulusTimeSeconds = 0.0;
	/** Last Increase Time Seconds 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	double LastIncreaseTimeSeconds = 0.0;
	/** Has Confirmed Sight 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	bool bHasConfirmedSight = false;
	/** Searching 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	bool bSearching = false;
	/** Observing 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	bool bObserving = false;
	/** First Increase In Band 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	bool bFirstIncreaseInBand = false;
	/** Decay Timer 的运行时句柄，用于取消回调并避免 Tick；不在蓝图中配置。 */
	FTimerHandle DecayTimer;
	/** Observation Timer 的运行时句柄，用于取消回调并避免 Tick；不在蓝图中配置。 */
	FTimerHandle ObservationTimer;
};
