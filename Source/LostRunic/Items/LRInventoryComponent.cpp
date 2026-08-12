/**
 * @file LRInventoryComponent.cpp
 * @brief 按稳定物品 ID 保存堆叠条目（数量 + 单调获得顺序）、武器选择、笔记与收藏品 ID；只维护物品状态和武器选择，不理解攻击条件或使用入口。
 *
 * 关联文件：LRInventoryComponent.h；所属领域：Items。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Items/LRInventoryComponent.h"

#include "Core/LRGameplayTags.h"
#include "Core/LRLog.h"
#include "Data/LRGameContentSet.h"
#include "Data/LRItemDefinition.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/LRGameInstanceSubsystem.h"

/**
 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
 */
ULRInventoryComponent::ULRInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

/**
 * @brief 在进入世界后解析运行时依赖、绑定事件并启动所需计时器；构造阶段不访问 World 或玩家对象。
 */
void ULRInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	const UGameInstance* gameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const ULRGameInstanceSubsystem* subsystem = gameInstance ? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	const ULRGameContentSet* contentSet = subsystem ? subsystem->GetContentSet() : nullptr;
	TArray<ULRItemDefinition*> definitions;
	if (contentSet)
	{
		for (ULRItemDefinition* definition : contentSet->Items)
		{
			definitions.Add(definition);
		}
	}
	InitializeDefinitions(definitions);
}

/**
 * @brief 按稳定物品 ID 建立定义索引，供库存查询、标签汇总和物品使用。
 * @param definitions 数据或调优来源 `definitions`；调用期间只读，并按稳定 ID 解析内容。
 */
void ULRInventoryComponent::InitializeDefinitions(const TArray<ULRItemDefinition*>& definitions)
{
	Definitions.Reset();
	for (ULRItemDefinition* definition : definitions)
	{
		if (definition && !definition->ItemId.IsNone())
		{
			Definitions.Add(definition->ItemId, definition);
		}
	}
}

/**
 * @brief 按稳定物品 ID 增加库存数量；达到 MaxStackSize 时返回 InventoryFull 且不改变库存。
 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
 * @param count 本次操作使用的计数、增量或索引 `count`；由函数校验合法范围。
 * @return Success、InventoryFull、InvalidDefinition 或 InvalidQuantity 的结构化结果。
 */
ELRAddItemResult ULRInventoryComponent::AddItem(const FName itemId, const int32 count)
{
	if (itemId.IsNone() || !Definitions.Contains(itemId))
	{
		return ELRAddItemResult::InvalidDefinition;
	}
	if (count <= 0)
	{
		return ELRAddItemResult::InvalidQuantity;
	}
	const ULRItemDefinition* definition = FindDefinition(itemId);
	const int32 currentQuantity = GetItemCount(itemId);
	if (currentQuantity + count > definition->MaxStackSize)
	{
		UE_LOG(LogLostRunicInteraction, Warning, TEXT("Inventory=%s cannot add item=%s count=%d; current=%d max=%d."),
			*GetNameSafe(GetOwner()), *itemId.ToString(), count, currentQuantity, definition->MaxStackSize);
		return ELRAddItemResult::InventoryFull;
	}

	FLRInventoryEntry& entry = Entries.FindOrAdd(itemId);
	entry.ItemId = itemId;
	if (entry.Quantity == 0)
	{
		entry.AcquisitionSequence = NextAcquisitionSequence++;
	}
	entry.Quantity += count;
	OnInventoryChanged.Broadcast(itemId, entry.Quantity);
	return ELRAddItemResult::Success;
}

/**
 * @brief 查询 Item Count；不修改领域状态。
 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
int32 ULRInventoryComponent::GetItemCount(const FName itemId) const
{
	const FLRInventoryEntry* entry = FindEntry(itemId);
	return entry ? entry->Quantity : 0;
}

/**
 * @brief 判断 Has Item 对应条件；不产生玩法副作用。
 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
 * @param count 本次操作使用的计数、增量或索引 `count`；由函数校验合法范围。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRInventoryComponent::HasItem(const FName itemId, const int32 count) const
{
	return count > 0 && GetItemCount(itemId) >= count;
}

/**
 * @brief 查询 Item Count；不修改领域状态。
 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
const FLRInventoryEntry* ULRInventoryComponent::FindEntry(const FName itemId) const
{
	const FLRInventoryEntry* entry = Entries.Find(itemId);
	return entry && entry->Quantity > 0 ? entry : nullptr;
}

/**
 * @brief 把玩家明确选择的武器设置为指定已持有且带 Item.Category.Weapon 的物品；None 是合法状态。
 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRInventoryComponent::SetSelectedWeapon(const FName itemId)
{
	if (itemId.IsNone())
	{
		SelectedWeaponItemId = NAME_None;
		return true;
	}
	if (!IsWeapon(itemId))
	{
		return false;
	}
	SelectedWeaponItemId = itemId;
	return true;
}

/**
 * @brief 查询 Selected Weapon；未选择或选择已失效时返回 None。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FName ULRInventoryComponent::GetSelectedWeapon() const
{
	if (SelectedWeaponItemId.IsNone() || !IsWeapon(SelectedWeaponItemId))
	{
		return NAME_None;
	}
	return SelectedWeaponItemId;
}

/**
 * @brief 查询攻击实际使用的武器：显式选择仍有效时返回它，否则按 AcquisitionSequence 返回最早获得的现存武器，没有武器时返回 None。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FName ULRInventoryComponent::GetEffectiveWeapon() const
{
	if (const FName selected = GetSelectedWeapon(); !selected.IsNone())
	{
		return selected;
	}
	FName earliestWeapon = NAME_None;
	int64 earliestSequence = MAX_int64;
	for (const TPair<FName, FLRInventoryEntry>& item : Entries)
	{
		if (item.Value.Quantity <= 0 || !IsWeapon(item.Key))
		{
			continue;
		}
		if (item.Value.AcquisitionSequence < earliestSequence)
		{
			earliestSequence = item.Value.AcquisitionSequence;
			earliestWeapon = item.Key;
		}
	}
	return earliestWeapon;
}

/**
 * @brief 把稳定笔记 ID 加入已读集合；集合语义保证重复阅读不会产生重复记录。
 * @param noteId 稳定标识 `noteId`；用于内容查询和存档，不依赖显示名或数组序号。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRInventoryComponent::AddNoteId(const FName noteId)
{
	if (noteId.IsNone() || NoteIds.Contains(noteId))
	{
		return false;
	}
	NoteIds.Add(noteId);
	return true;
}

/**
 * @brief 把稳定收藏品 ID 加入已收集集合；重复添加返回 AlreadyOwned 且不修改世界状态。
 * @param collectibleId 稳定标识 `collectibleId`；用于内容查询和存档，不依赖显示名或数组序号。
 * @return Success、AlreadyOwned 或 InvalidDefinition 的结构化结果。
 */
ELRAddCollectibleResult ULRInventoryComponent::AddCollectibleId(const FName collectibleId)
{
	if (collectibleId.IsNone())
	{
		return ELRAddCollectibleResult::InvalidDefinition;
	}
	if (CollectibleIds.Contains(collectibleId))
	{
		return ELRAddCollectibleResult::AlreadyOwned;
	}
	CollectibleIds.Add(collectibleId);
	return ELRAddCollectibleResult::Success;
}

/**
 * @brief 查询 Owned Item Tags；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FGameplayTagContainer ULRInventoryComponent::GetOwnedItemTags() const
{
	FGameplayTagContainer tags;
	for (const TPair<FName, FLRInventoryEntry>& item : Entries)
	{
		if (item.Value.Quantity > 0)
		{
			const ULRItemDefinition* definition = FindDefinition(item.Key);
			if (definition)
			{
				tags.AppendTags(definition->ItemTags);
			}
		}
	}
	return tags;
}

/**
 * @brief 查询 Owned Item Ids；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
TArray<FName> ULRInventoryComponent::GetOwnedItemIds() const
{
	TArray<FName> itemIds;
	for (const TPair<FName, FLRInventoryEntry>& item : Entries)
	{
		if (item.Value.Quantity > 0)
		{
			itemIds.Add(item.Key);
		}
	}
	itemIds.Sort(FNameLexicalLess());
	return itemIds;
}

/**
 * @brief 查询 Note Ids；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
TArray<FName> ULRInventoryComponent::GetNoteIds() const
{
	TArray<FName> noteIds = NoteIds.Array();
	noteIds.Sort(FNameLexicalLess());
	return noteIds;
}

/**
 * @brief 查询 Collectible Ids；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
TArray<FName> ULRInventoryComponent::GetCollectibleIds() const
{
	TArray<FName> collectibleIds = CollectibleIds.Array();
	collectibleIds.Sort(FNameLexicalLess());
	return collectibleIds;
}

/**
 * @brief 按稳定 ID 或运行时条件查找 Definition，未找到时返回明确失败值。
 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
ULRItemDefinition* ULRInventoryComponent::FindDefinition(const FName itemId) const
{
	const TObjectPtr<ULRItemDefinition>* definition = Definitions.Find(itemId);
	return definition ? definition->Get() : nullptr;
}

/**
 * @brief 把运行时库存、笔记或收藏品状态复制到存档分块；不填充已废弃的快捷栏字段。
 * @param outInventory 参与本次操作的运行时对象 `outInventory`；函数会检查空值和所需接口。
 */
void ULRInventoryComponent::CaptureSaveState(FLRSaveInventoryChunk& outInventory) const
{
	outInventory.ItemCounts.Reset();
	for (const TPair<FName, FLRInventoryEntry>& item : Entries)
	{
		if (item.Value.Quantity > 0)
		{
			outInventory.ItemCounts.Add(item.Key, item.Value.Quantity);
		}
	}
	outInventory.NoteIds = NoteIds;
	outInventory.CollectibleIds = CollectibleIds;
}

/**
 * @brief 从存档分块恢复运行时状态；完全忽略已废弃的快捷栏字段。
 * @param savedInventory 参与本次操作的运行时对象 `savedInventory`；函数会检查空值和所需接口。
 */
void ULRInventoryComponent::RestoreSaveState(const FLRSaveInventoryChunk& savedInventory)
{
	Entries.Reset();
	SelectedWeaponItemId = NAME_None;
	for (const TPair<FName, int32>& item : savedInventory.ItemCounts)
	{
		if (item.Value > 0 && Definitions.Contains(item.Key))
		{
			FLRInventoryEntry& entry = Entries.Add(item.Key);
			entry.ItemId = item.Key;
			entry.Quantity = item.Value;
			entry.AcquisitionSequence = NextAcquisitionSequence++;
		}
	}
	NoteIds = savedInventory.NoteIds;
	CollectibleIds = savedInventory.CollectibleIds;

	for (const TPair<FName, FLRInventoryEntry>& item : Entries)
	{
		OnInventoryChanged.Broadcast(item.Key, item.Value.Quantity);
	}
}

/**
 * @brief 在事务成功后扣除一个一次性物品；扣为 0 时删除条目并清理显式武器选择。
 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRInventoryComponent::TryConsumeItem(const FName itemId)
{
	FLRInventoryEntry* entry = Entries.Find(itemId);
	if (!entry || entry->Quantity <= 0)
	{
		return false;
	}
	--entry->Quantity;
	if (entry->Quantity <= 0)
	{
		RemoveEntry(itemId);
		return true;
	}
	OnInventoryChanged.Broadcast(itemId, entry->Quantity);
	return true;
}

/**
 * @brief 从库存删除条目；若该条目是当前显式武器则清空选择。
 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
 */
void ULRInventoryComponent::RemoveEntry(const FName itemId)
{
	if (Entries.Remove(itemId) > 0)
	{
		if (SelectedWeaponItemId == itemId)
		{
			SelectedWeaponItemId = NAME_None;
		}
		OnInventoryChanged.Broadcast(itemId, 0);
	}
}

/**
 * @brief 判断 Is Weapon 对应条件；不产生玩法副作用。
 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRInventoryComponent::IsWeapon(const FName itemId) const
{
	const ULRItemDefinition* definition = FindDefinition(itemId);
	return HasItem(itemId) && definition && definition->ItemTags.HasTag(LRGameplayTags::ItemCategoryWeapon);
}
