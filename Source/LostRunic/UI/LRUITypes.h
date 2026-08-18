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

class UTexture2D;

/** 通用 UI 命令；Screen 基类只理解这些命令、方向导航与焦点生命周期，不理解背包、笔记或收藏品等具体页面类型。 */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic UI Command"))
enum class ELRUICommand : uint8
{
	Confirm UMETA(DisplayName = "Confirm"),
	Cancel UMETA(DisplayName = "Cancel"),
	PreviousTab UMETA(DisplayName = "Previous Tab"),
	NextTab UMETA(DisplayName = "Next Tab"),
	PrimaryAction UMETA(DisplayName = "Primary Action"),
	Delete UMETA(DisplayName = "Delete")
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Screen Type"))
enum class ELRScreenType : uint8
{
	None UMETA(DisplayName = "None"),
	HUD UMETA(DisplayName = "HUD"),
	StateOverlay UMETA(DisplayName = "State Overlay"),
	Dialogue UMETA(DisplayName = "Dialogue"),
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

	/** 物品图标；由快照构建阶段同步加载，Widget 侧不加载资源。 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UTexture2D> Icon;
};

/** 统一菜单笔记页的单条笔记视图；Locked 笔记只暴露 ReadingId 与本地化“？？？”，不暴露真实标题或正文。 */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Note View"))
struct LOSTRUNIC_API FLRNoteView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Journal")
	FName ReadingId = NAME_None;

	/** 未解锁时为本地化“？？？”，已解锁时为真实标题。 */
	UPROPERTY(BlueprintReadOnly, Category = "Journal")
	FText Title;

	/** 未解锁时为空，不泄露正文。 */
	UPROPERTY(BlueprintReadOnly, Category = "Journal")
	FText Body;

	UPROPERTY(BlueprintReadOnly, Category = "Journal")
	bool bUnlocked = false;
};

/** 统一菜单收藏品页的单条收藏品视图；Locked 条目只暴露稳定 ID、剪影图与解锁标记，不暴露真实名称、描述或真实图标。 */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Collectible View"))
struct LOSTRUNIC_API FLRCollectibleView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Collectibles")
	FName CollectibleId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Collectibles")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Collectibles")
	FText Description;

	/** 未解锁时为定义 LockedIcon 或 UI 调优共享剪影；已解锁时为真实图标。由快照构建阶段同步加载。 */
	UPROPERTY(BlueprintReadOnly, Category = "Collectibles")
	TObjectPtr<UTexture2D> Icon;

	UPROPERTY(BlueprintReadOnly, Category = "Collectibles")
	bool bUnlocked = false;
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

	/** 只读 UI View Model：背包、笔记、收藏品与武器选择。Widget 只能消费快照，所有动作必须回到 ULRInventoryComponent。 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TArray<FLRInventoryItemView> Items;

	/** 笔记页条目（含 Locked 占位）；按 ReadingId 字典序。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TArray<FLRNoteView> Notes;

	/** 收藏品页条目（含 Locked 占位）；按 DisplayOrder 再按 CollectibleId。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TArray<FLRCollectibleView> Collectibles;

	/** 玩家明确选择的武器稳定 ID；None 表示未显式选择。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FName SelectedWeaponItemId = NAME_None;

	/** 攻击实际使用的武器稳定 ID（含按获得顺序的自动回退）；本界面标识不使用它。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FName EffectiveWeaponItemId = NAME_None;

	/** 快照是否可用于展示；容量越界、缺库存等失败场景为 false，UI 必须 fail closed。 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	bool bIsValid = false;
};
