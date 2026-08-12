/**
 * @file LRGameContentSet.h
 * @brief 聚合 Dialogue/Reading 表、物品/收藏品/守卫/关卡事件定义和地图软引用注册，避免运行时代码散落硬编码 /Game 路径。
 *
 * 关联文件：LRGameContentSet.cpp；所属领域：Data。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "CoreMinimal.h"
#include "Data/LRContentRows.h"
#include "Engine/DataAsset.h"

#include "LRGameContentSet.generated.h"

class UDataTable;
class ULRCollectibleDefinition;
class ULRGuardDefinition;
class ULRItemDefinition;
class ULRLevelEventDefinition;

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Game Content Set"))
class LOSTRUNIC_API ULRGameContentSet : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Dialogue Table DataTable 引用；行以稳定 FName ID 查询。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Content|Tables")
	TObjectPtr<UDataTable> DialogueTable;

	/** Reading Table DataTable 引用；行以稳定 FName ID 查询。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Content|Tables")
	TObjectPtr<UDataTable> ReadingTable;

	/** Items 的领域数据，由所属类型负责维护和校验。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Content|Definitions")
	TArray<TObjectPtr<ULRItemDefinition>> Items;

	/** Collectibles 的领域数据，由所属类型负责维护和校验。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Content|Definitions")
	TArray<TObjectPtr<ULRCollectibleDefinition>> Collectibles;

	/** Guards 的领域数据，由所属类型负责维护和校验。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Content|Definitions")
	TArray<TObjectPtr<ULRGuardDefinition>> Guards;

	/** Level Events 的领域数据，由所属类型负责维护和校验。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Content|Definitions")
	TArray<TObjectPtr<ULRLevelEventDefinition>> LevelEvents;

	/** Maps 的领域数据，由所属类型负责维护和校验。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Content|Maps")
	TArray<FLRMapRegistration> Maps;

	/**
	 * @brief 校验当前资产的必填引用、数值边界及跨字段关系，并输出可诊断错误。
	 * @param outError 输出校验失败原因；成功时保持为空。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool Validate(FString& outError) const;

	/**
	 * @brief 按稳定 ID 或运行时条件查找 Map，未找到时返回明确失败值。
	 * @param mapId 稳定标识 `mapId`；用于内容查询和存档，不依赖显示名或数组序号。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Content")
	TSoftObjectPtr<UWorld> FindMap(FName mapId) const;

	/** Finds an item definition by its stable ItemId. */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Content")
	ULRItemDefinition* FindItemDefinition(FName itemId) const;

	/** Finds a collectible definition by its stable CollectibleId. */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Content")
	ULRCollectibleDefinition* FindCollectibleDefinition(FName collectibleId) const;

	/** Finds a guard definition by its stable GuardId. */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Content")
	ULRGuardDefinition* FindGuardDefinition(FName guardId) const;

	/**
	 * @brief 按稳定 ID 或运行时条件查找 Map Id For World，未找到时返回明确失败值。
	 * @param world 要解析地图 ID、应用恢复状态或执行查询的 Unreal World。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	FName FindMapIdForWorld(const UWorld* world) const;

#if WITH_EDITOR
	/**
	 * @brief 接入 Unreal Data Validation，将领域校验错误报告给编辑器。
	 * @param context 用于本次条件匹配的 `context` 标签或上下文。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif
};
