/**
 * @file LRUITypes.h
 * @brief 实现 HUD、状态遮罩、对话/阅读、背包/笔记/收藏、暂停、存档槽和过场的控制器边界。UI 订阅领域事件并负责表现，不参与核心规则判定。
 *
 * 关联文件：UI 目录内调用该公共契约的实现文件；所属领域：UI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Core/LRTypes.h"
#include "Narrative/LRNarrativeTypes.h"

#include "LRUITypes.generated.h"

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Screen Type"))
enum class ELRScreenType : uint8
{
	None UMETA(DisplayName = "None"),
	HUD UMETA(DisplayName = "HUD"),
	StateOverlay UMETA(DisplayName = "State Overlay"),
	Narrative UMETA(DisplayName = "Narrative"),
	Journal UMETA(DisplayName = "Journal"),
	Inventory UMETA(DisplayName = "Inventory"),
	Collectibles UMETA(DisplayName = "Collectibles"),
	Pause UMETA(DisplayName = "Pause"),
	SaveSlots UMETA(DisplayName = "Save Slots"),
	Transition UMETA(DisplayName = "Transition")
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Narrative Presentation"))
struct LOSTRUNIC_API FLRNarrativePresentation
{
	GENERATED_BODY()

	/** Page 的领域数据，由所属类型负责维护和校验。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative")
	FLRNarrativePage Page;

	/** Displayed Text 的领域数据，由所属类型负责维护和校验。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative")
	FText DisplayedText;

	/** Text Fully Revealed 的开关；true 表示启用，false 表示禁用。 C++ 安全默认值为 `false`。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative")
	bool bTextFullyRevealed = false;
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Inventory Snapshot"))
struct LOSTRUNIC_API FLRInventorySnapshot
{
	GENERATED_BODY()

	/** Item Ids 的领域数据，由所属类型负责维护和校验。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TArray<FName> ItemIds;

	/** 已阅读笔记的稳定 ID 集合。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TArray<FName> NoteIds;

	/** 已取得收藏品的稳定 ID 集合。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TArray<FName> CollectibleIds;

	/** 四个快捷栏保存的物品稳定 ID，空槽使用 NAME_None。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TArray<FName> QuickSlots;

	/** Selected Quick Slot 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `0`。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 SelectedQuickSlot = 0;
};
