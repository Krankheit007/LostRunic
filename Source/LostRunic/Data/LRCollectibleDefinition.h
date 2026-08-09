/**
 * @file LRCollectibleDefinition.h
 * @brief 定义 LostRunic 的内容数据和调优 DataAsset。设计文档中的速度、距离、角度、持续时间、冷却及表现强度都由这里提供编辑器权威值，C++ 默认值仅作安全回退。
 *
 * 关联文件：LRCollectibleDefinition.cpp；所属领域：Data。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "LRCollectibleDefinition.generated.h"

class UStaticMesh;
class UTexture2D;

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Collectible Definition"))
class LOSTRUNIC_API ULRCollectibleDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Collectible Id 的稳定 FName/GUID 标识；用于定义查询和存档，不依赖显示名或临时 Actor 名称。 C++ 安全默认值为 `NAME_None`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Collectible")
	FName CollectibleId = NAME_None;

	/** Display Name 的领域数据，由所属类型负责维护和校验。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Collectible")
	FText DisplayName;

	/** Description 的领域数据，由所属类型负责维护和校验。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Collectible", meta = (MultiLine = "true"))
	FText Description;

	/** Display Mesh 的领域数据，由所属类型负责维护和校验。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Collectible")
	TSoftObjectPtr<UStaticMesh> DisplayMesh;

	/** Icon 的领域数据，由所属类型负责维护和校验。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Collectible")
	TSoftObjectPtr<UTexture2D> Icon;

	/** Display Order 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `0`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：最小值 `0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Collectible", meta = (ClampMin = "0"))
	int32 DisplayOrder = 0;

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
