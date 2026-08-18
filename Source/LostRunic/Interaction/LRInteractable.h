/**
 * @file LRInteractable.h
 * @brief 实现统一交互契约：按距离、总朝向角、遮挡和当前状态筛选唯一目标，并以结构化选项和结果连接 UI、背包选物及可交互对象。
 *
 * 关联文件：Interaction 目录内调用该公共契约的实现文件；所属领域：Interaction。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Interaction/LRInteractionTypes.h"
#include "UObject/Interface.h"

#include "LRInteractable.generated.h"

class USceneComponent;

UINTERFACE(BlueprintType, meta = (DisplayName = "Lost Runic Interactable"))
class LOSTRUNIC_API ULRInteractable : public UInterface
{
	GENERATED_BODY()
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
class LOSTRUNIC_API ILRInteractable
{
	GENERATED_BODY()

public:
	/**
	 * @brief 查询 Interaction Options；不修改领域状态。
	 * @param interactor 参与本次操作的运行时对象 `interactor`；函数会检查空值和所需接口。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Lost Runic|Interaction")
	TArray<FLRInteractionOption> GetInteractionOptions(AActor* interactor);

	/**
	 * @brief 查询 Interaction Location；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Lost Runic|Interaction")
	FVector GetInteractionLocation();

	/**
	 * Resolves the optional world component used only as the HUD prompt anchor.
	 * This never participates in interaction eligibility, scoring, traces, or target selection.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Lost Runic|Interaction|Presentation")
	USceneComponent* GetInteractionPromptAnchorComponent();

	/**
	 * @brief 实现 Execute Interaction 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 * @param interactor 参与本次操作的运行时对象 `interactor`；函数会检查空值和所需接口。
	 * @param actionTag Gameplay Tag 或标签集合，用于分类、条件、拒绝原因和可诊断事件。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Lost Runic|Interaction")
	FLRInteractionResult ExecuteInteraction(AActor* interactor, FGameplayTag actionTag);
};
