/**
 * @file LRNPCDefinition.h
 * @brief 通用 NPC 的内容定义 DataAsset：StateTree 硬引用、默认行为与对话行 ID；公共调优在 ULRNPCTuning，巡逻点按实例配置。
 *
 * 关联文件：LRNPCDefinition.cpp；所属领域：Data。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "AI/LRNPCTypes.h"
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "LRNPCDefinition.generated.h"

class UStateTree;

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic NPC Definition"))
class LOSTRUNIC_API ULRNPCDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Npc Id 的稳定 FName/GUID 标识；用于定义查询和存档，不依赖显示名或临时 Actor 名称。 C++ 安全默认值为 `NAME_None`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NPC")
	FName NpcId = NAME_None;

	/** Behavior 的 StateTree 硬引用；随定义资产同步加载，OnPossess 时确定启动顺序。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NPC")
	TObjectPtr<UStateTree> Behavior;

	/** Default Behavior 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `ENPCBaseBehavior::Idle`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NPC")
	ENPCBaseBehavior DefaultBehavior = ENPCBaseBehavior::Idle;

	/**
	 * @brief 查询 Primary Asset Id；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

#if WITH_EDITOR
	/**
	 * @brief 接入 Unreal Data Validation，将领域校验错误报告给编辑器。
	 * @param context 用于本次条件匹配的 `context` 标签或上下文。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif
};
