/**
 * @file LRInteractionComponent.h
 * @brief 以可调计时器扫描 Interaction 通道，分别计算 20 m 提示、5 m 描边和 2 m 执行状态，并向 HUD 发布唯一焦点。
 *
 * 关联文件：LRInteractionComponent.cpp；所属领域：Interaction。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Components/ActorComponent.h"
#include "Interaction/LRInteractionTypes.h"

#include "LRInteractionComponent.generated.h"

class ULRInteractionTuning;
class ULRInventoryComponent;
class ULRInteractionPresentationComponent;
class ULRStateComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FLRInteractionTargetChanged, AActor*, target,
	FLRInteractionOption, option, ELRInteractionRange, range);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRInteractionExecuted, FLRInteractionResult, result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRFocusedInteractionChanged, FLRInteractionPromptView, promptView);

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(ClassGroup = "Lost Runic", BlueprintType, meta = (BlueprintSpawnableComponent, DisplayName = "Lost Runic Interaction"))
class LOSTRUNIC_API ULRInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 */
	ULRInteractionComponent();

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
	 * @brief 对当前唯一候选重新校验距离、朝向和遮挡后执行主交互，并返回结构化结果。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Interaction")
	FLRInteractionResult PerformPrimaryInteraction();

	/** Re-evaluates nearby interactables immediately after execution or a meaningful world-state change. */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Interaction")
	void RefreshInteractionState();

	/**
	 * @brief 查询 Current Target；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Interaction")
	AActor* GetCurrentTarget() const { return CurrentTarget.Get(); }

	/**
	 * @brief 查询 Current Option；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Interaction")
	FLRInteractionOption GetCurrentOption() const { return CurrentOption; }

	/** Returns the HUD data for the only actor currently allowed to receive primary interaction. */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Interaction")
	FLRInteractionPromptView GetFocusedPrompt() const { return CurrentPrompt; }

	/**
	 * @brief 查询 Current Range；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Interaction")
	ELRInteractionRange GetCurrentRange() const { return CurrentRange; }

	/**
	 * @brief 向对应日志分类输出当前状态、配置来源和关键运行时值，供 LR.Debug 命令诊断。
	 */
	void LogDiagnostics() const;

	/** 当 Target Changed 发生时广播；蓝图可绑定该委托以更新表现，不应在回调中改写核心规则。  */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Interaction")
	FLRInteractionTargetChanged OnTargetChanged;

	/** 当 Interaction Executed 发生时广播；蓝图可绑定该委托以更新表现，不应在回调中改写核心规则。  */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Interaction")
	FLRInteractionExecuted OnInteractionExecuted;

	/** Published only by the interaction component; HUD subscribers never search world actors themselves. */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Interaction")
	FLRFocusedInteractionChanged OnFocusedInteractionChanged;

private:
	struct FEvaluation
	{
		TWeakObjectPtr<AActor> Actor;
		FLRInteractionOption Option;
		FLRInteractionCandidateScore Score;
		float ExecuteDistance = 0.0f;
		bool bShowHint = false;
		bool bCanExecute = false;
		ELRInteractionPresentationState PresentationState = ELRInteractionPresentationState::None;
	};

	/**
	 * @brief 按距离、朝向、遮挡和当前状态筛选交互候选，并只保留唯一最优目标。
	 */
	void ScanCandidates();
	/** Builds evaluation values without producing traces or presentation side effects. */
	void BuildEvaluations(TArray<FEvaluation>& outEvaluations) const;
	/** Applies presentation state to every visible actor independently from the unique Focus target. */
	void ApplyPresentationStates(const TArray<FEvaluation>& evaluations, int32 focusedIndex);
	/** Selects the nearest executable, facing candidate that is not occluded. */
	int32 SelectFocusedEvaluation(TArray<FEvaluation>& evaluations) const;
	/**
	 * @brief 判断 Is Occluded 对应条件；不产生玩法副作用。
	 * @param target 本次规则检查或操作的目标对象。
	 * @param targetLocation 空间值 `targetLocation`；距离和位置使用 Unreal 厘米单位。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool IsOccluded(AActor* target, const FVector& targetLocation) const;
	/**
	 * @brief 把 Apply Selection 数据应用到运行时对象，并显式处理缺失依赖。
	 * @param candidates 本次领域操作的结构化数据 `candidates`；字段语义由对应 USTRUCT 定义。
	 * @param selectedIndex 本次操作使用的计数、增量或索引 `selectedIndex`；由函数校验合法范围。
	 */
	void ApplySelection(const TArray<FEvaluation>& evaluations, int32 selectedIndex);
	/**
	 * @brief 查询 Effective Tuning；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	const ULRInteractionTuning& GetEffectiveTuning() const;

	/** 运行时解析出的调优资产缓存；不序列化，不由蓝图编辑。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TObjectPtr<ULRInteractionTuning> Tuning;

	/** Inventory 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TObjectPtr<ULRInventoryComponent> Inventory;

	/** State 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TObjectPtr<ULRStateComponent> State;

	/** Current Target 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	TWeakObjectPtr<AActor> CurrentTarget;
	/** Current Option 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	FLRInteractionOption CurrentOption;
	/** Current Range 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	ELRInteractionRange CurrentRange = ELRInteractionRange::None;
	/** Last prompt supplied to the HUD. Target is weak to avoid owning a world actor from UI. */
	FLRInteractionPromptView CurrentPrompt;
	/** Components changed by the prior scan; reset before the next state map is applied. */
	TArray<TWeakObjectPtr<ULRInteractionPresentationComponent>> PresentedComponents;
	/** Query Timer 的运行时句柄，用于取消回调并避免 Tick；不在蓝图中配置。 */
	FTimerHandle QueryTimer;
};
