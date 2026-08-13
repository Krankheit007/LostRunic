/**
 * @file LRMenuWidgetController.cpp
 * @brief 统一菜单控制器：维护背包/笔记/收集品/暂停/存档槽 Tab 状态，绑定 Inventory 与内容定义，构建不含快捷栏数据的只读库存快照；交互选物模式下计算物品与目标兼容性。
 *
 * 关联文件：LRMenuWidgetController.h；所属领域：UI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "UI/LRMenuWidgetController.h"

#include "Core/LRGameplayTags.h"
#include "Core/LRLog.h"
#include "CoreGlobals.h"
#include "Data/LRCollectibleDefinition.h"
#include "Data/LRContentRows.h"
#include "Data/LRGameContentSet.h"
#include "Data/LRItemDefinition.h"
#include "Data/LRUITuning.h"
#include "GameFramework/Actor.h"
#include "Items/LRInventoryComponent.h"
#include "Items/LRItemUseTarget.h"

/**
 * @brief 绑定库存、内容定义与 UI 调优，并订阅库存领域事件；重复绑定同一库存时不做任何事。
 * @param inventory 参与本次操作的运行时对象 `inventory`；函数会检查空值和所需接口。
 * @param contentSet 数据或调优来源 `contentSet`；调用期间只读，并按稳定 ID 解析内容。
 * @param uiTuning 数据或调优来源 `uiTuning`；调用期间只读，并按稳定 ID 解析内容。
 */
void ULRMenuWidgetController::Initialize(ULRInventoryComponent* inventory, ULRGameContentSet* contentSet, ULRUITuning* uiTuning)
{
	if (!inventory || Inventory == inventory)
	{
		return;
	}
	Inventory = inventory;
	ContentSet = contentSet;
	UITuning = uiTuning;
	Inventory->OnInventoryChanged.AddDynamic(this, &ULRMenuWidgetController::HandleInventoryChanged);
	Inventory->OnNotesChanged.AddDynamic(this, &ULRMenuWidgetController::HandleNotesChanged);
	Inventory->OnCollectiblesChanged.AddDynamic(this, &ULRMenuWidgetController::HandleCollectiblesChanged);
	Inventory->OnSelectedWeaponChanged.AddDynamic(this, &ULRMenuWidgetController::HandleSelectedWeaponChanged);
}

/**
 * @brief 切换到指定菜单页面并广播变化；同一时刻只保留一个可见菜单。打开/切换时立即重建并广播快照。
 * @param screen 本次操作使用的 `screen` 枚举或模式值。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRMenuWidgetController::OpenScreen(const ELRScreenType screen)
{
	if (screen != ELRScreenType::Journal && screen != ELRScreenType::Inventory && screen != ELRScreenType::Collectibles
		&& screen != ELRScreenType::Pause && screen != ELRScreenType::SaveSlots)
	{
		return false;
	}
	const ELRScreenType previousScreen = OpenScreenType;
	OpenScreenType = screen;
	OnMenuScreenChanged.Broadcast(previousScreen, OpenScreenType);
	if (OpenScreenType == ELRScreenType::Journal || OpenScreenType == ELRScreenType::Inventory
		|| OpenScreenType == ELRScreenType::Collectibles)
	{
		RebuildSnapshot();
	}
	return true;
}

/**
 * @brief 关闭当前菜单页面并广播页面变化，焦点由 PlayerController 重新分配。
 */
void ULRMenuWidgetController::CloseScreen()
{
	if (OpenScreenType == ELRScreenType::None)
	{
		return;
	}
	const ELRScreenType previousScreen = OpenScreenType;
	OpenScreenType = ELRScreenType::None;
	OnMenuScreenChanged.Broadcast(previousScreen, OpenScreenType);
}

/**
 * @brief 构建统一菜单的库存快照；传入 itemUseTarget 时计算每件物品与目标的兼容性（交互选物模式）。
 *        只读 View Model：Widget 只能消费，所有动作回到 InventoryComponent。
 * @param inventory 参与本次操作的运行时对象 `inventory`；函数会检查空值和所需接口。
 * @param itemUseTarget 本次规则检查或操作的目标对象；交互选物模式下传入，普通浏览传 nullptr。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FLRInventorySnapshot ULRMenuWidgetController::BuildInventorySnapshot(const ULRInventoryComponent* inventory,
	AActor* itemUseTarget) const
{
	FLRInventorySnapshot snapshot;
	if (!inventory)
	{
		return snapshot;
	}
	snapshot.bIsValid = true;

	// 容量是开发期契约：越界必须 fail closed，绝不静默展示前 N 条。
	const bool bWithinCapacity = inventory->GetOwnedEntriesOrdered().Num() <= LRMenuCapacity::Bag
		&& inventory->GetNoteIds().Num() <= LRMenuCapacity::Notes
		&& inventory->GetCollectibleIds().Num() <= LRMenuCapacity::Collectibles;
	if (!bWithinCapacity)
	{
		// ensure 的自定义消息不进入日志，无法被自动化测试断言匹配；自动化环境走 UE_LOG(Error) 断言。
		if (!GIsAutomationTesting)
		{
			ensureAlwaysMsgf(false, TEXT("Inventory=%s exceeds menu capacity; snapshot is invalid (fail closed)."),
				*GetNameSafe(inventory->GetOwner()));
		}
		UE_LOG(LogLostRunicUI, Error, TEXT("Menu snapshot invalid: inventory=%s exceeds capacity (bag=%d note=%d collectible=%d)."),
			*GetNameSafe(inventory->GetOwner()), LRMenuCapacity::Bag, LRMenuCapacity::Notes, LRMenuCapacity::Collectibles);
		snapshot.bIsValid = false;
		return snapshot;
	}

	snapshot.SelectedWeaponItemId = inventory->GetSelectedWeapon();
	snapshot.EffectiveWeaponItemId = inventory->GetEffectiveWeapon();

	FGameplayTagContainer targetTags;
	if (itemUseTarget && itemUseTarget->GetClass()->ImplementsInterface(ULRItemUseTarget::StaticClass()))
	{
		targetTags = ILRItemUseTarget::Execute_GetItemUseTargetTags(itemUseTarget);
	}

	// 背包：按获得顺序再按 ItemId（快照构建阶段唯一允许的同步加载点）。
	for (const FLRInventoryEntry& entry : inventory->GetOwnedEntriesOrdered())
	{
		const ULRItemDefinition* definition = inventory->FindDefinition(entry.ItemId);
		if (!definition)
		{
			continue;
		}
		FLRInventoryItemView& view = snapshot.Items.AddDefaulted_GetRef();
		view.ItemId = entry.ItemId;
		view.DisplayName = definition->DisplayName.IsEmpty() ? FText::FromName(entry.ItemId) : definition->DisplayName;
		view.Description = definition->Description;
		view.Quantity = entry.Quantity;
		view.bConsumable = definition->bConsumable;
		view.bIsWeapon = definition->ItemTags.HasTag(LRGameplayTags::ItemCategoryWeapon);
		view.bIsSelectedWeapon = view.bIsWeapon && snapshot.SelectedWeaponItemId == entry.ItemId;
		view.Icon = definition->Icon.LoadSynchronous();
		if (targetTags.IsValid())
		{
			view.bCompatibleWithTarget = definition->AllowedActionTags.HasTag(LRGameplayTags::InteractionActionUse)
				&& targetTags.HasAny(definition->AllowedTargetTags);
			view.FailureReason = view.bCompatibleWithTarget ? FGameplayTag() : LRGameplayTags::InteractionRejectItem;
		}
	}

	// 笔记：ReadingTable 全行（含 Locked 占位），按 ReadingId 字典序；Locked 只暴露稳定 ID 与“？？？”。
	if (ContentSet && ContentSet->ReadingTable)
	{
		TArray<FLRReadingRow> readingRows;
		for (const FName rowName : ContentSet->ReadingTable->GetRowNames())
		{
			if (const FLRReadingRow* row = ContentSet->ReadingTable->FindRow<FLRReadingRow>(rowName, TEXT("Build menu snapshot")))
			{
				readingRows.Add(*row);
			}
		}
		BuildNoteViews(readingRows, *inventory, snapshot.Notes);
	}

	// 收藏品：定义全量（含 Locked 剪影占位），按 DisplayOrder 再按 CollectibleId。
	if (ContentSet)
	{
		TArray<ULRCollectibleDefinition*> orderedCollectibles;
		for (const TObjectPtr<ULRCollectibleDefinition>& definition : ContentSet->Collectibles)
		{
			orderedCollectibles.Add(definition.Get());
		}
		// TArray<指针>::Sort 对元素解引用后调用谓词，因此谓词接收对象引用。
		orderedCollectibles.Sort([](const ULRCollectibleDefinition& a, const ULRCollectibleDefinition& b)
		{
			if (a.DisplayOrder != b.DisplayOrder)
			{
				return a.DisplayOrder < b.DisplayOrder;
			}
			return a.CollectibleId.LexicalLess(b.CollectibleId);
		});
		for (const ULRCollectibleDefinition* definition : orderedCollectibles)
		{
			if (!definition)
			{
				continue;
			}
			FLRCollectibleView& view = snapshot.Collectibles.AddDefaulted_GetRef();
			view.CollectibleId = definition->CollectibleId;
			view.bUnlocked = inventory->GetCollectibleIds().Contains(definition->CollectibleId);
			if (view.bUnlocked)
			{
				view.DisplayName = definition->DisplayName;
				view.Description = definition->Description;
				view.Icon = definition->Icon.LoadSynchronous();
			}
			else
			{
				// Locked 只暴露剪影图；定义缺失时回退到 UI 调优的共享剪影。
				const TSoftObjectPtr<UTexture2D>& lockedIcon = !definition->LockedIcon.IsNull()
					? definition->LockedIcon
					: (UITuning ? UITuning->LockedCollectibleIcon : TSoftObjectPtr<UTexture2D>());
				view.Icon = lockedIcon.LoadSynchronous();
			}
		}
	}

	// 构建后复查：内容侧（ReadingTable 行数、收藏品定义数）同样不得超出共享容量。
	if (snapshot.Notes.Num() > LRMenuCapacity::Notes || snapshot.Collectibles.Num() > LRMenuCapacity::Collectibles)
	{
		if (!GIsAutomationTesting)
		{
			ensureAlwaysMsgf(false, TEXT("Content exceeds menu capacity; snapshot is invalid (fail closed)."));
		}
		UE_LOG(LogLostRunicUI, Error, TEXT("Menu snapshot invalid: content exceeds capacity (notes=%d collectibles=%d)."),
			snapshot.Notes.Num(), snapshot.Collectibles.Num());
		snapshot.bIsValid = false;
		snapshot.Items.Reset();
		snapshot.Notes.Reset();
		snapshot.Collectibles.Reset();
	}
	return snapshot;
}

/**
 * @brief 由阅读表行构建笔记视图（含 Locked 占位），按 ReadingId 字典序；Locked 只暴露稳定 ID 与“？？？”。
 *        独立静态纯函数：DataTable 迭代只是外壳，该规则可被自动化测试直接覆盖。
 * @param rows 本次领域操作的结构化数据 `rows`；字段语义由对应 USTRUCT 定义。
 * @param inventory 参与本次操作的运行时对象 `inventory`；函数会检查空值和所需接口。
 * @param outViews 输出笔记视图数组；调用前内容被清空。
 */
void ULRMenuWidgetController::BuildNoteViews(const TArray<FLRReadingRow>& rows, const ULRInventoryComponent& inventory,
	TArray<FLRNoteView>& outViews)
{
	outViews.Reset();
	for (const FLRReadingRow& row : rows)
	{
		FLRNoteView& view = outViews.AddDefaulted_GetRef();
		view.ReadingId = row.ReadingId;
		view.bUnlocked = inventory.GetNoteIds().Contains(row.ReadingId);
		if (view.bUnlocked)
		{
			view.Title = row.Title;
			view.Body = row.Body;
		}
		else
		{
			view.Title = NSLOCTEXT("LRMenu", "LockedNoteTitle", "？？？");
		}
	}
	outViews.Sort([](const FLRNoteView& a, const FLRNoteView& b)
	{
		return a.ReadingId.LexicalLess(b.ReadingId);
	});
}

/**
 * @brief 装备指定武器；合法性由 InventoryComponent 校验，成功后经领域事件驱动快照重建与广播。
 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRMenuWidgetController::EquipSelectedWeapon(const FName itemId)
{
	if (!Inventory)
	{
		return false;
	}
	return Inventory->SetSelectedWeapon(itemId);
}

/**
 * @brief 库存条目变化：菜单打开时立即重建并广播，否则只标记 dirty。
 */
void ULRMenuWidgetController::HandleInventoryChanged(const FName itemId, const int32 newCount)
{
	MarkSnapshotDirty();
}

/**
 * @brief 笔记集合变化：菜单打开时立即重建并广播，否则只标记 dirty。
 */
void ULRMenuWidgetController::HandleNotesChanged()
{
	MarkSnapshotDirty();
}

/**
 * @brief 收藏品集合变化：菜单打开时立即重建并广播，否则只标记 dirty。
 */
void ULRMenuWidgetController::HandleCollectiblesChanged()
{
	MarkSnapshotDirty();
}

/**
 * @brief 武器选择变化：菜单打开时立即重建并广播，否则只标记 dirty。
 */
void ULRMenuWidgetController::HandleSelectedWeaponChanged()
{
	MarkSnapshotDirty();
}

/**
 * @brief 标记快照 dirty；菜单打开或可见期间收到领域事件时立即重建并广播。
 */
void ULRMenuWidgetController::MarkSnapshotDirty()
{
	bSnapshotDirty = true;
	if (OpenScreenType == ELRScreenType::Journal || OpenScreenType == ELRScreenType::Inventory
		|| OpenScreenType == ELRScreenType::Collectibles)
	{
		RebuildSnapshot();
	}
}

/**
 * @brief 用当前库存重建快照并广播 OnSnapshotChanged；清空 dirty 标记。
 */
void ULRMenuWidgetController::RebuildSnapshot()
{
	CachedSnapshot = BuildInventorySnapshot(Inventory);
	bSnapshotDirty = false;
	OnSnapshotChanged.Broadcast(CachedSnapshot);
}
