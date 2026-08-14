/**
 * @file LRGuardAIController.h
 * @brief 把 AI Perception 的 Sight/Hearing 事件转换为警戒原因标签，并驱动 Idle、Suspicious、Investigate、Search、Chase 行为、导航速度和捕获检测。
 *
 * 关联文件：LRGuardAIController.cpp；所属领域：AI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "AIController.h"
#include "AI/LRGuardTypes.h"
#include "Perception/AIPerceptionTypes.h"

#include "LRGuardAIController.generated.h"

class ALRGuardCharacter;
class UAIPerceptionComponent;
class UAISenseConfig_Hearing;
class UAISenseConfig_Sight;
class ULRAlertComponent;
class ULRGuardTuning;
class ULRStateTuning;
class UStateTreeAIComponent;

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Guard AI Controller"))
class LOSTRUNIC_API ALRGuardAIController : public AAIController
{
	GENERATED_BODY()

public:
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 */
	ALRGuardAIController();

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
	 * @brief 处理 On Possess 事件，将引擎回调转换为对应领域状态更新。
	 * @param inPawn Controller 新接管的 Pawn；期望为 ALRGuardCharacter。
	 */
	virtual void OnPossess(APawn* inPawn) override;
	/**
	 * @brief 处理 On Un Possess 事件，将引擎回调转换为对应领域状态更新。
	 */
	virtual void OnUnPossess() override;

	/**
	 * @brief 查询 Alert Component；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI")
	ULRAlertComponent* GetAlertComponent() const { return Alert.Get(); }

	/**
	 * @brief 进入指定守卫行为，设置移动速度、焦点、导航目标或搜索超时。
	 * @param behavior 要进入或退出的守卫 StateTree 行为状态。
	 */
	void EnterBehavior(ELRGuardBehaviorState behavior);
	/**
	 * @brief 退出指定守卫行为并清理该状态拥有的导航、焦点或计时器。
	 * @param behavior 要进入或退出的守卫 StateTree 行为状态。
	 */
	void ExitBehavior(ELRGuardBehaviorState behavior);
	/**
	 * @brief 查询 Resolved Behavior；行为状态唯一权威解析（眩晕优先，否则警戒推导），StateTree 只执行该结果。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI")
	ELRGuardBehaviorState GetResolvedBehavior() const;

	/**
	 * @brief 输出守卫行为、警戒值和最后异常点，并按调试开关绘制视野与听觉范围。
	 */
	void LogAndDrawDiagnostics() const;

protected:
	/**
	 * @brief 处理 On Move Completed 事件，将引擎回调转换为对应领域状态更新。
	 * @param requestId 稳定标识 `requestId`；用于内容查询和存档，不依赖显示名或数组序号。
	 * @param result 本次领域操作的结构化数据 `result`；字段语义由对应 USTRUCT 定义。
	 */
	virtual void OnMoveCompleted(FAIRequestID requestId, const FPathFollowingResult& result) override;

private:
	/**
	 * @brief 把 UE 感知刺激转换为可见/听见事件、异常位置和警戒原因标签。
	 * @param actor 本次查询、交互或事件涉及的 Actor。
	 * @param stimulus 时间值 `stimulus`，单位为秒。
	 */
	UFUNCTION()
	void HandlePerception(AActor* actor, FAIStimulus stimulus);

	/**
	 * @brief 处理 Handle Alert Changed 事件，将引擎回调转换为对应领域状态更新。
	 * @param previousLevel 本次操作使用的计数、增量或索引 `previousLevel`；由函数校验合法范围。
	 * @param currentLevel 本次操作使用的计数、增量或索引 `currentLevel`；由函数校验合法范围。
	 * @param currentState 本次操作使用的 `currentState` 枚举或模式值。
	 * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
	 * @param disturbanceLocation 空间值 `disturbanceLocation`；距离和位置使用 Unreal 厘米单位。
	 */
	UFUNCTION()
	void HandleAlertChanged(int32 previousLevel, int32 currentLevel, ELRGuardBehaviorState currentState,
		FGameplayTag reason, FVector disturbanceLocation);

	/**
	 * @brief 用 Guard 调优资产配置 UE Sight/Hearing 感知，包括完整视野角换算、距离和阵营检测。
	 */
	void ConfigurePerception();
	/**
	 * @brief 按可调低频计时检查追逐目标距离；进入捕获半径后触发玩家死亡与 Memory 流程；眩晕期间跳过。
	 */
	void HandleCaptureTimer();
	/**
	 * @brief 处理 Handle Knockback 事件：进入 Stunned 覆盖（停止移动、清除焦点），按 Courage 击退时长计时恢复。
	 * @param direction 击退方向 `direction`；仅用于诊断。
	 */
	UFUNCTION()
	void HandleKnockback(FVector direction);
	/**
	 * @brief 眩晕计时结束后按当前警戒与视线重新解析行为并广播恢复事件。
	 */
	void HandleStunEnd();
	/**
	 * @brief 开始 Start Patrol Move 流程，建立本次操作拥有的状态、委托或计时器。
	 */
	void StartPatrolMove();
	/**
	 * @brief 判断 Can Confirm Sight 对应条件；不产生玩法副作用。
	 * @param actor 本次查询、交互或事件涉及的 Actor。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool CanConfirmSight(AActor* actor) const;
	/**
	 * @brief 判断 Is Hidden From Guard 对应条件；不产生玩法副作用。
	 * @param actor 本次查询、交互或事件涉及的 Actor。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool IsHiddenFromGuard(AActor* actor) const;
	/**
	 * @brief 查询 Effective Tuning；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	const ULRGuardTuning& GetEffectiveTuning() const;

	/** State Tree AI 的领域数据，由所属类型负责维护和校验。 仅在蓝图或详情面板中查看，不可编辑。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStateTreeAIComponent> StateTreeAI;

	/** AIPerception 的领域数据，由所属类型负责维护和校验。 仅在蓝图或详情面板中查看，不可编辑。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAIPerceptionComponent> AIPerception;

	/** Sight Config 的领域数据，由所属类型负责维护和校验。  */
	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	/** Hearing Config 的领域数据，由所属类型负责维护和校验。  */
	UPROPERTY()
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	/** 运行时解析出的调优资产缓存；不序列化，不由蓝图编辑。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TObjectPtr<ULRGuardTuning> Tuning;

	/** State 调优缓存；眩晕时长与 Courage 击退根运动同源。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TObjectPtr<ULRStateTuning> StateTuning;

	/** Alert 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TWeakObjectPtr<ULRAlertComponent> Alert;

	/** Active Behavior 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	ELRGuardBehaviorState ActiveBehavior = ELRGuardBehaviorState::IdlePatrol;
	/** Patrol Index 的内部运行时数据；不参与蓝图配置。 */
	int32 PatrolIndex = 0;
	/** Stunned 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	bool bStunned = false;
	/** Capture Timer 的运行时句柄，用于取消回调并避免 Tick；不在蓝图中配置。 */
	FTimerHandle CaptureTimer;
	/** Stun Timer 的运行时句柄，用于取消回调并避免 Tick；不在蓝图中配置。 */
	FTimerHandle StunTimer;
};
