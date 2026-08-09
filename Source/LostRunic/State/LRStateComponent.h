/**
 * @file LRStateComponent.h
 * @brief 维护玩家当前心理状态、眼部输入先按者优先门控、0.8 秒进入/0.3 秒返回长按和表现安全锁；关联 LRStateRules、LRStateTuning、LRStatePresentationComponent 与 LRPlayerController。
 *
 * 关联文件：LRStateComponent.cpp；所属领域：State。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "State/LRStateRules.h"

#include "LRStateComponent.generated.h"

class ULRStateTuning;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FLRStateChanging, ELRPerceptionMode, previousMode,
	ELRPerceptionMode, nextMode, FGameplayTag, reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLRStateChanged, ELRPerceptionMode, currentMode, FGameplayTag, reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLRStateChangeRejected, FLRStateChangeRequest, request, FGameplayTag, reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FLRStateHoldStarted, ELRStateRequestType, inputType,
	ELRPerceptionMode, targetMode, float, holdSeconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRStateHoldCanceled, ELRStateRequestType, inputType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLRStateHoldThresholdReached, ELRStateRequestType, inputType,
	ELRPerceptionMode, targetMode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRPresentationLockChanged, bool, bLocked);

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(ClassGroup = "Lost Runic", BlueprintType, meta = (BlueprintSpawnableComponent, DisplayName = "Lost Runic State"))
class LOSTRUNIC_API ULRStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 */
	ULRStateComponent();

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
	 * @brief 校验并提交四状态切换请求；成功时广播切换前后事件并启动表现锁，失败时返回原因标签。
	 * @param request 不可变领域请求，包含本次操作所需的稳定 ID、来源、目标或原因。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|State")
	FLRStateChangeResult RequestStateChange(const FLRStateChangeRequest& request);

	/**
	 * @brief 记录一次闭眼或睁眼按下，按先按者优先规则确定目标状态，并使用调优时长启动一次性长按计时。
	 * @param inputType 闭眼或睁眼输入语义，不包含具体键位。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|State|Input")
	void BeginEyeInput(ELRStateRequestType inputType);

	/**
	 * @brief 结束指定眼部输入；未达到阈值时取消请求，已提交的同一次按下不会重复触发。
	 * @param inputType 闭眼或睁眼输入语义，不包含具体键位。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|State|Input")
	void EndEyeInput(ELRStateRequestType inputType);

	/**
	 * @brief 取消当前眼部输入事务并清空门控，供菜单、对话、死亡或过场接管输入时调用。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|State|Input")
	void CancelEyeInputSequence();

	/**
	 * @brief 按来源标签启用或解除状态输入阻塞；多个系统可独立持有阻塞，互不覆盖。
	 * @param blocker 标识菜单、对话、过场、死亡等输入阻塞来源的 Gameplay Tag。
	 * @param bActive 为 true 时加入阻塞或启用状态，为 false 时移除或停用。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|State")
	void SetBlockerActive(FGameplayTag blocker, bool bActive);

	/**
	 * @brief 接收 UMG、动画或表现层的完成回调并释放状态锁；安全超时只处理资源异常。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|State|Presentation")
	void NotifyPresentationComplete();

	/**
	 * @brief 查询 Current Mode；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|State")
	ELRPerceptionMode GetCurrentMode() const { return CurrentMode; }

	/**
	 * @brief 判断 Is Presentation Locked 对应条件；不产生玩法副作用。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|State")
	bool IsPresentationLocked() const { return bPresentationLocked; }

	/**
	 * @brief 查询 Last Transition Reason；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|State")
	FGameplayTag GetLastTransitionReason() const { return LastTransitionReason; }

	/**
	 * @brief 查询 Active Blockers；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|State")
	FGameplayTagContainer GetActiveBlockers() const { return ActiveBlockers; }

	/**
	 * @brief 向对应日志分类输出当前状态、配置来源和关键运行时值，供 LR.Debug 命令诊断。
	 */
	void LogDiagnostics() const;

	/** 当 State Changing 发生时广播；蓝图可绑定该委托以更新表现，不应在回调中改写核心规则。  */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|State")
	FLRStateChanging OnStateChanging;

	/** 当 State Changed 发生时广播；蓝图可绑定该委托以更新表现，不应在回调中改写核心规则。  */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|State")
	FLRStateChanged OnStateChanged;

	/** 当 State Change Rejected 发生时广播；蓝图可绑定该委托以更新表现，不应在回调中改写核心规则。  */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|State")
	FLRStateChangeRejected OnStateChangeRejected;

	/** 当 Hold Started 发生时广播；蓝图可绑定该委托以更新表现，不应在回调中改写核心规则。  */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|State|Input")
	FLRStateHoldStarted OnHoldStarted;

	/** 当 Hold Canceled 发生时广播；蓝图可绑定该委托以更新表现，不应在回调中改写核心规则。  */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|State|Input")
	FLRStateHoldCanceled OnHoldCanceled;

	/** 当 Hold Threshold Reached 发生时广播；蓝图可绑定该委托以更新表现，不应在回调中改写核心规则。  */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|State|Input")
	FLRStateHoldThresholdReached OnHoldThresholdReached;

	/** 当 Presentation Lock Changed 发生时广播；蓝图可绑定该委托以更新表现，不应在回调中改写核心规则。  */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|State|Presentation")
	FLRPresentationLockChanged OnPresentationLockChanged;

private:
	/**
	 * @brief 在长按达到调优阈值时构造并提交唯一状态请求。
	 */
	void HandleHoldThreshold();
	/**
	 * @brief 在表现资源未回调时执行安全解锁并记录异常，避免永久阻塞输入。
	 */
	void HandlePresentationSafetyTimeout();
	/**
	 * @brief 统一构造状态拒绝结果、记录原因标签并广播 Rejected 事件。
	 * @param request 不可变领域请求，包含本次操作所需的稳定 ID、来源、目标或原因。
	 * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
	 */
	void RejectRequest(const FLRStateChangeRequest& request, FGameplayTag reason);
	/**
	 * @brief 开始 Start Presentation Lock 流程，建立本次操作拥有的状态、委托或计时器。
	 */
	void StartPresentationLock();
	/**
	 * @brief 查询 Effective Tuning；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	const ULRStateTuning& GetEffectiveTuning() const;

	/** 当前已提交的心理状态；仅状态组件可修改，蓝图只能读取。 C++ 安全默认值为 `ELRPerceptionMode::Normal`。 仅在蓝图或详情面板中查看，不可编辑。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	ELRPerceptionMode CurrentMode = ELRPerceptionMode::Normal;

	/** 运行时解析出的调优资产缓存；不序列化，不由蓝图编辑。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TObjectPtr<ULRStateTuning> Tuning;

	/** 当前禁止状态请求的原因标签集合，如对话、菜单、过场或死亡。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	FGameplayTagContainer ActiveBlockers;

	/** Last Transition Reason 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	FGameplayTag LastTransitionReason;
	/** Input Gate 的内部运行时数据；不参与蓝图配置。 */
	FLRStateInputGate InputGate;
	/** Pending Input Target 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	ELRPerceptionMode PendingInputTarget = ELRPerceptionMode::Normal;
	/** Presentation Locked 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	bool bPresentationLocked = false;
	/** Hold Timer 的运行时句柄，用于取消回调并避免 Tick；不在蓝图中配置。 */
	FTimerHandle HoldTimer;
	/** Presentation Safety Timer 的运行时句柄，用于取消回调并避免 Tick；不在蓝图中配置。 */
	FTimerHandle PresentationSafetyTimer;
};
