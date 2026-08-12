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
#include "GameplayTagContainer.h"
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

/** 统一菜单背包页的物品条目视图；数量只对一次性物品显示，无限物品不显示数字。 */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Inventory Item View"))
struct LOSTRUNIC_API FLRInventoryItemView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FName ItemId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FText Description;

	/** 一次性物品显示为剩余使用次数；无限物品为 1 但 UI 应隐藏数字。 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = "0"))
	int32 Quantity = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	bool bConsumable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	bool bIsWeapon = false;

	/** 玩家明确选择的当前武器标记；自动回退武器不写入该字段。 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	bool bIsSelectedWeapon = false;

	/** 交互选物模式下该物品与当前目标的兼容性；未进入选物模式时为 false。 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	bool bCompatibleWithTarget = false;

	/** 兼容性检查的内部失败原因标签；UI 必须映射为友好提示，不得直接显示给普通玩家。 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FGameplayTag FailureReason;
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

	/** 背包页条目；数量只对一次性物品显示。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TArray<FLRInventoryItemView> Items;

	/** 已阅读笔记的稳定 ID 集合。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TArray<FName> NoteIds;

	/** 已取得收藏品的稳定 ID 集合。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TArray<FName> CollectibleIds;

	/** 玩家明确选择的武器稳定 ID；None 表示未显式选择。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FName SelectedWeaponItemId = NAME_None;

	/** 攻击实际使用的武器稳定 ID（含按获得顺序的自动回退）；None 表示攻击将空手执行。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FName EffectiveWeaponItemId = NAME_None;
};
