/**
 * @file LRNPCController.h
 * @brief 通用 NPC 控制器：Hearing 感知驱动噪声反应（Conversation 高优先级不被打断）、StateTree 生命周期（OnPossess 解析定义后启动）、巡逻与低频玩家朝向检测；不实现第二套计时器状态机。
 *
 * 关联文件：LRNPCController.cpp；所属领域：AI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "AI/LRNPCTypes.h"
#include "AIController.h"
#include "GameplayTagContainer.h"
#include "Perception/AIPerceptionTypes.h"

#include "LRNPCController.generated.h"

class ALRNPCCharacter;
class UAIPerceptionComponent;
class UAISenseConfig_Hearing;
class ULRNPCDefinition;
class ULRNPCTuning;
class UStateTreeAIComponent;

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic NPC AI Controller"))
class LOSTRUNIC_API ALRNPCController : public AAIController
{
	GENERATED_BODY()

public:
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 */
	ALRNPCController();

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
	 * @brief 处理 On Possess 事件：解析定义、SetStateTree 后 StartLogic，并绑定 Hearing 感知。
	 * @param inPawn Controller 新接管的 Pawn；期望为 ALRNPCCharacter。
	 */
	virtual void OnPossess(APawn* inPawn) override;
	/**
	 * @brief 处理 On Un Possess 事件：解绑感知并停止 StateTree 逻辑。
	 */
	virtual void OnUnPossess() override;
	/**
	 * @brief 处理 On Move Completed 事件：巡逻点到达续走下一段。
	 * @param requestId 稳定标识 `requestId`；用于内容查询和存档，不依赖显示名或数组序号。
	 * @param result 本次领域操作的结构化数据 `result`；字段语义由对应 USTRUCT 定义。
	 */
	virtual void OnMoveCompleted(FAIRequestID requestId, const FPathFollowingResult& result) override;

	/**
	 * @brief 进入指定 NPC 行为：Idle 启动玩家朝向检测、Patrol 巡逻、ReactToNoise 转向声源限时反应、Conversation 停止一切反应。
	 * @param behavior 要进入或退出的 NPC StateTree 行为状态。
	 */
	void EnterBehavior(ELRNPCBehaviorState behavior);
	/**
	 * @brief 退出指定 NPC 行为并清理该状态拥有的导航、焦点或计时器。
	 * @param behavior 要进入或退出的 NPC StateTree 行为状态。
	 */
	void ExitBehavior(ELRNPCBehaviorState behavior);
	/**
	 * @brief 查询 Active Behavior；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	ELRNPCBehaviorState GetActiveBehavior() const { return ActiveBehavior; }

	/**
	 * @brief 启动/停止 Idle 低频玩家朝向检测（任务节点调用）。
	 */
	void StartLookAtTimer();
	/**
	 * @brief 停止 Idle 低频玩家朝向检测（任务节点调用）。
	 */
	void StopLookAtTimer();
	/**
	 * @brief 开始限时噪声反应：转向声源并按 NoiseReactionDurationSeconds 计时，结束后发送 NPCReactionEnded 事件。
	 * @param location 世界空间位置，Unreal 单位为厘米。
	 */
	void StartNoiseReaction(const FVector location);
	/**
	 * @brief 结束噪声反应计时（任务退出时调用）。
	 */
	void StopNoiseReaction();

	/**
	 * @brief 对话开始：进入 Conversation（高优先级），停止一切反应。
	 */
	void NotifyDialogueStarted();
	/**
	 * @brief 对话结束：回到配置的默认行为。
	 */
	void NotifyDialogueEnded();
	/**
	 * @brief 查询 Last Noise Location；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	FVector GetLastNoiseLocation() const { return LastNoiseLocation; }

private:
	/**
	 * @brief 把 UE 听觉刺激转换为噪声反应；Conversation 期间只触发表现钩子，不切换行为。
	 * @param actor 本次查询、交互或事件涉及的 Actor。
	 * @param stimulus 时间值 `stimulus`，单位为秒。
	 */
	UFUNCTION()
	void HandlePerception(AActor* actor, FAIStimulus stimulus);

	/**
	 * @brief 处理 Handle Look At Timer 事件：距离、视线与可达性满足时朝向玩家。
	 */
	void HandleLookAtTimer();
	/**
	 * @brief 处理 Handle Reaction Timeout 事件：噪声反应结束，发送 NPCReactionEnded。
	 */
	void HandleReactionTimeout();
	/**
	 * @brief 开始 Start Patrol Move 流程，建立本次操作拥有的状态、委托或计时器。
	 */
	void StartPatrolMove();
	/**
	 * @brief 解析配置的默认行为到运行态枚举。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	ELRNPCBehaviorState GetBaseBehavior() const;
	/**
	 * @brief 向 StateTree 发送行为事件；树未运行时直接进入行为。
	 * @param event 本次领域操作的结构化数据 `event`；字段语义由对应 USTRUCT 定义。
	 * @param behavior 要进入或退出的 NPC StateTree 行为状态。
	 */
	void DispatchBehaviorEvent(const FGameplayTag event, ELRNPCBehaviorState behavior);
	/**
	 * @brief 查询 Effective Tuning；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	const ULRNPCTuning& GetEffectiveTuning() const;

	/** State Tree AI 的领域数据，由所属类型负责维护和校验。 仅在蓝图或详情面板中查看，不可编辑。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStateTreeAIComponent> StateTreeAI;

	/** AIPerception 的领域数据，由所属类型负责维护和校验。 仅在蓝图或详情面板中查看，不可编辑。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAIPerceptionComponent> AIPerception;

	/** Hearing Config 的领域数据，由所属类型负责维护和校验。  */
	UPROPERTY()
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	/** 运行时解析出的调优资产缓存；不序列化，不由蓝图编辑。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TObjectPtr<ULRNPCTuning> Tuning;

	/** Definition 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TWeakObjectPtr<ULRNPCDefinition> Definition;

	/** Npc 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TWeakObjectPtr<ALRNPCCharacter> Npc;

	/** Active Behavior 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	ELRNPCBehaviorState ActiveBehavior = ELRNPCBehaviorState::Idle;
	/** Patrol Index 的内部运行时数据；不参与蓝图配置。 */
	int32 PatrolIndex = 0;
	/** Last Noise Location 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	FVector LastNoiseLocation = FVector::ZeroVector;
	/** Look At Timer 的运行时句柄，用于取消回调并避免 Tick；不在蓝图中配置。 */
	FTimerHandle LookAtTimer;
	/** Reaction Timer 的运行时句柄，用于取消回调并避免 Tick；不在蓝图中配置。 */
	FTimerHandle ReactionTimer;
};
