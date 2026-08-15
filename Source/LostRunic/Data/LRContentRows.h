/**
 * @file LRContentRows.h
 * @brief 定义 LostRunic 的内容数据和调优 DataAsset。设计文档中的速度、距离、角度、持续时间、冷却及表现强度都由这里提供编辑器权威值，C++ 默认值仅作安全回退。
 *
 * 关联文件：Data 目录内调用该公共契约的实现文件；所属领域：Data。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"

#include "LRContentRows.generated.h"

class UTexture2D;
class UWorld;

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Dialogue Option"))
struct LOSTRUNIC_API FLRDialogueOption
{
	GENERATED_BODY()

	/** Option Id 的稳定 FName/GUID 标识；用于定义查询和存档，不依赖显示名或临时 Actor 名称。 C++ 安全默认值为 `NAME_None`。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName OptionId = NAME_None;

	/** Text 的领域数据，由所属类型负责维护和校验。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FText Text;

	/** Next Row Id 的稳定 FName/GUID 标识；用于定义查询和存档，不依赖显示名或临时 Actor 名称。 C++ 安全默认值为 `NAME_None`。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName NextRowId = NAME_None;

	/** Required Tags 的 Gameplay Tag 条件或分类，用于数据驱动规则与诊断。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Conditions")
	FGameplayTagContainer RequiredTags;

	/** Blocked Tags 的 Gameplay Tag 条件或分类，用于数据驱动规则与诊断。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Conditions")
	FGameplayTagContainer BlockedTags;
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Dialogue Row"))
struct LOSTRUNIC_API FLRDialogueRow : public FTableRowBase
{
	GENERATED_BODY()

	/** Dialogue Id 的稳定 FName/GUID 标识；用于定义查询和存档，不依赖显示名或临时 Actor 名称。 C++ 安全默认值为 `NAME_None`。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName DialogueId = NAME_None;

	/** Speaker Id 的稳定 FName/GUID 标识；用于定义查询和存档，不依赖显示名或临时 Actor 名称。 C++ 安全默认值为 `NAME_None`。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName SpeakerId = NAME_None;

	/** Text 的领域数据，由所属类型负责维护和校验。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue", meta = (MultiLine = "true"))
	FText Text;

	/** Portrait 的领域数据，由所属类型负责维护和校验。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TSoftObjectPtr<UTexture2D> Portrait;

	/** Next Row Id 的稳定 FName/GUID 标识；用于定义查询和存档，不依赖显示名或临时 Actor 名称。 C++ 安全默认值为 `NAME_None`。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName NextRowId = NAME_None;

	/** Options 的领域数据，由所属类型负责维护和校验。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TArray<FLRDialogueOption> Options;

	/** Required Tags 的 Gameplay Tag 条件或分类，用于数据驱动规则与诊断。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Conditions")
	FGameplayTagContainer RequiredTags;

	/** Blocked Tags 的 Gameplay Tag 条件或分类，用于数据驱动规则与诊断。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Conditions")
	FGameplayTagContainer BlockedTags;
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Reading Row"))
struct LOSTRUNIC_API FLRReadingRow : public FTableRowBase
{
	GENERATED_BODY()

	/** Reading Id 的稳定 FName/GUID 标识；用于定义查询和存档，不依赖显示名或临时 Actor 名称。 C++ 安全默认值为 `NAME_None`。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reading")
	FName ReadingId = NAME_None;

	/** Title 的领域数据，由所属类型负责维护和校验。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reading")
	FText Title;

	/** Body 的开关；true 表示启用，false 表示禁用。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reading", meta = (MultiLine = "true"))
	FText Body;

	/** Chapter Tags 的 Gameplay Tag 条件或分类，用于数据驱动规则与诊断。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reading")
	FGameplayTagContainer ChapterTags;
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Map Registration"))
struct LOSTRUNIC_API FLRMapRegistration
{
	GENERATED_BODY()

	/** Map Id 的稳定 FName/GUID 标识；用于定义查询和存档，不依赖显示名或临时 Actor 名称。 C++ 安全默认值为 `NAME_None`。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	FName MapId = NAME_None;

	/** Stable key in the ContentSet UI string table; the only authored source for the localized map name. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|Localization")
	FName DisplayNameTextKey = NAME_None;

	/** Only playable maps accumulate play time and request post-travel autosaves. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	bool bPlayableMap = true;

	/** Stable fallback anchor used for new games and saves without a usable transform. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	FName DefaultStartAnchorId = NAME_None;

	/** World 的领域数据，由所属类型负责维护和校验。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	TSoftObjectPtr<UWorld> World;
};
