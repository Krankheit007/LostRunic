/**
 * @file LRNPCCharacter.h
 * @brief 通用非战斗 NPC：由 StateTree（Idle/Patrol/ReactToNoise/Conversation）驱动；实现对话交互（Talk 选项经 ULRDialogueSubsystem::StartDialogue）与噪声表现钩子（OnNoiseHeard / OnNPCAttentionChanged 预留未来告警/逃离）。
 *
 * 关联文件：LRNPCCharacter.cpp；所属领域：AI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "AI/LRNPCTypes.h"
#include "GameFramework/Character.h"
#include "Interaction/LRInteractable.h"

#include "LRNPCCharacter.generated.h"

class ALRNPCController;
class ULRNPCDefinition;
class ULRDialogueSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLRNPCAttentionChanged, FVector, location, FGameplayTag, reason);

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic NPC Character"))
class LOSTRUNIC_API ALRNPCCharacter : public ACharacter, public ILRInteractable
{
	GENERATED_BODY()

public:
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 */
	ALRNPCCharacter();

	/**
	 * @brief 查询 Definition；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|NPC")
	ULRNPCDefinition* GetDefinition() const { return Definition; }

	/**
	 * @brief 查询 Patrol Point；不修改领域状态。
	 * @param index 目标元素索引，调用前必须满足对应容器边界。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	AActor* GetPatrolPoint(int32 index) const;
	/**
	 * @brief 查询 Patrol Point Count；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	int32 GetPatrolPointCount() const { return PatrolPoints.Num(); }

	/**
	 * @brief 通知 NPC 听见噪声：触发表现钩子与预留委托；Conversation 高优先级时由控制器决定是否切换行为。
	 * @param location 世界空间位置，Unreal 单位为厘米。
	 * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
	 */
	void NotifyNoiseHeard(const FVector location, const FGameplayTag reason);

	/**
	 * @brief 查询当前 NPC 行为（由控制器权威解析）；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|NPC")
	ELRNPCBehaviorState GetActiveBehavior() const;

	//~ ILRInteractable
	virtual TArray<FLRInteractionOption> GetInteractionOptions_Implementation(AActor* interactor) override;
	virtual FVector GetInteractionLocation_Implementation() override;
	virtual FLRInteractionResult ExecuteInteraction_Implementation(AActor* interactor, FGameplayTag actionTag) override;
	//~ End ILRInteractable

	/** 当 Noise Heard 发生时广播；蓝图可绑定该委托以更新表现（转向、表情等），不应在回调中改写核心规则。  */
	UFUNCTION(BlueprintImplementableEvent, Category = "Lost Runic|NPC")
	void OnNoiseHeard(const FVector location, const FGameplayTag reason);

	/** 当 NPC Attention Changed 发生时广播；预留未来告警/逃离扩展钩子，本次不实现告警逻辑。  */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|NPC")
	FLRNPCAttentionChanged OnNPCAttentionChanged;

protected:
	/** Definition 的领域数据，由所属类型负责维护和校验。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NPC")
	TObjectPtr<ULRNPCDefinition> Definition;

	/** Patrol Points 的领域数据，由所属类型负责维护和校验。 可在关卡中的蓝图实例详情面板配置。 */
	UPROPERTY(EditInstanceOnly, Category = "NPC|Patrol")
	TArray<TObjectPtr<AActor>> PatrolPoints;

private:
	/**
	 * @brief 处理 Handle Dialogue Session Ended 事件，将引擎回调转换为对应领域状态更新。
	 * @param sessionType 本次操作使用的 `sessionType` 枚举或模式值。
	 * @param contentId 稳定标识 `contentId`；用于内容查询和存档，不依赖显示名或数组序号。
	 */
	UFUNCTION()
	void HandleDialogueSessionEnded(ELRNarrativeSessionType sessionType, FName contentId);
};
