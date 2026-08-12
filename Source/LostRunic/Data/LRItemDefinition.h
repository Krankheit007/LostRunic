/**
 * @file LRItemDefinition.h
 * @brief 定义 LostRunic 的内容数据和调优 DataAsset。设计文档中的速度、距离、角度、持续时间、冷却及表现强度都由这里提供编辑器权威值，C++ 默认值仅作安全回退。
 *
 * 关联文件：LRItemDefinition.cpp；所属领域：Data。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "LRItemDefinition.generated.h"

class UTexture2D;

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Item Definition"))
class LOSTRUNIC_API ULRItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Item Id 的稳定 FName/GUID 标识；用于定义查询和存档，不依赖显示名或临时 Actor 名称。 C++ 安全默认值为 `NAME_None`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FName ItemId = NAME_None;

	/** Display Name 的领域数据，由所属类型负责维护和校验。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	/** Description 的领域数据，由所属类型负责维护和校验。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FText Description;

	/** Icon 的领域数据，由所属类型负责维护和校验。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TSoftObjectPtr<UTexture2D> Icon;

	/** 一次性物品的每次成功使用消耗一个库存数量；false 表示无限使用且唯一持有。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Rules")
	bool bConsumable = false;

	/** 最大持有数量。一次性物品允许大于 1（库存数量即剩余使用次数）；无限使用物品必须为 1，数据校验拒绝其他值。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Rules",
		meta = (ClampMin = "1", ClampMax = "999", UIMin = "1", UIMax = "99"))
	int32 MaxStackSize = 1;

	/** Item Tags 的 Gameplay Tag 条件或分类，用于数据驱动规则与诊断。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Rules")
	FGameplayTagContainer ItemTags;

	/** Allowed Action Tags 只声明入口能力：`Interaction.Action.Use` 或 `Interaction.Action.Attack`；不负责状态、距离、朝向、目标有效性或攻击结果判定。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Rules")
	FGameplayTagContainer AllowedActionTags;

	/** Allowed Target Tags 的 Gameplay Tag 条件或分类，用于 Use 入口的目标兼容检查；攻击入口由 ILRAttackTarget 独立筛选。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Rules")
	FGameplayTagContainer AllowedTargetTags;

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
