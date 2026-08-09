/**
 * @file LRInventoryComponent.cpp
 * @brief 保存物品数量、4 格快捷栏、笔记及收藏品稳定 ID，并把快捷栏使用和背包选物统一提交给 LRItemUseResolver。
 *
 * 关联文件：LRInventoryComponent.h；所属领域：Items。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Items/LRInventoryComponent.h"

#include "Core/LRGameplayTags.h"
#include "Data/LRGameContentSet.h"
#include "Data/LRItemDefinition.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "Items/LRItemUseResolver.h"

namespace
{
	constexpr int32 QuickSlotCount = 4;
}

/**
 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
 */
ULRInventoryComponent::ULRInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	QuickSlots.SetNum(QuickSlotCount);
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
	Resolver = NewObject<ULRItemUseResolver>(this);
	Resolver->Initialize(this);
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
	if (!Resolver)
	{
		Resolver = NewObject<ULRItemUseResolver>(this);
		Resolver->Initialize(this);
	}
}

/**
 * @brief 按稳定物品 ID 增加库存数量；拒绝未知定义、非正数量和溢出结果。
 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
 * @param count 本次操作使用的计数、增量或索引 `count`；由函数校验合法范围。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRInventoryComponent::AddItem(const FName itemId, const int32 count)
{
	if (itemId.IsNone() || count <= 0 || !Definitions.Contains(itemId))
	{
		return false;
	}
	int32& currentCount = ItemCounts.FindOrAdd(itemId);
	currentCount += count;
	OnInventoryChanged.Broadcast(itemId, currentCount);
	return true;
}

/**
 * @brief 查询 Item Count；不修改领域状态。
 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
int32 ULRInventoryComponent::GetItemCount(const FName itemId) const
{
	const int32* count = ItemCounts.Find(itemId);
	return count ? *count : 0;
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
 * @brief 把已持有且允许快捷使用的物品 ID 分配到 0-3 快捷栏位。
 * @param slotIndex 槽位下标；快捷栏为 0-3，手动存档槽按调优上限校验。
 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRInventoryComponent::AssignQuickSlot(const int32 slotIndex, const FName itemId)
{
	if (!QuickSlots.IsValidIndex(slotIndex) || (!itemId.IsNone() && !HasItem(itemId)))
	{
		return false;
	}
	QuickSlots[slotIndex] = itemId;
	OnQuickSlotChanged.Broadcast(slotIndex, itemId);
	return true;
}

/**
 * @brief 执行 Select Quick Slot 的纯规则或事务判定，失败时提供结构化原因。
 * @param slotIndex 槽位下标；快捷栏为 0-3，手动存档槽按调优上限校验。
 */
void ULRInventoryComponent::SelectQuickSlot(const int32 slotIndex)
{
	if (QuickSlots.IsValidIndex(slotIndex))
	{
		SelectedQuickSlot = slotIndex;
	}
}

/**
 * @brief 执行 Select Adjacent Quick Slot 的纯规则或事务判定，失败时提供结构化原因。
 * @param direction 本次操作使用的计数、增量或索引 `direction`；由函数校验合法范围。
 */
void ULRInventoryComponent::SelectAdjacentQuickSlot(const int32 direction)
{
	SelectedQuickSlot = (SelectedQuickSlot + FMath::Sign(direction) + QuickSlotCount) % QuickSlotCount;
}

/**
 * @brief 查询 Quick Slot Item；不修改领域状态。
 * @param slotIndex 槽位下标；快捷栏为 0-3，手动存档槽按调优上限校验。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FName ULRInventoryComponent::GetQuickSlotItem(const int32 slotIndex) const
{
	return QuickSlots.IsValidIndex(slotIndex) ? QuickSlots[slotIndex] : NAME_None;
}

/**
 * @brief 从指定快捷栏取得物品并通过统一物品事务作用于当前目标。
 * @param slotIndex 槽位下标；快捷栏为 0-3，手动存档槽按调优上限校验。
 * @param target 本次规则检查或操作的目标对象。
 * @param currentMode 本次操作使用的 `currentMode` 枚举或模式值。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FLRItemUseResult ULRInventoryComponent::UseQuickSlot(const int32 slotIndex, AActor* target,
	const ELRPerceptionMode currentMode)
{
	const FName itemId = GetQuickSlotItem(slotIndex);
	return UseItem(BuildUseRequest(itemId, slotIndex, target, currentMode, ELRItemUseEntryPoint::QuickSlot));
}

/**
 * @brief 执行 Use Item From Selector 的玩法动作；输入层只提供语义，合法性由对应领域组件决定。
 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
 * @param target 本次规则检查或操作的目标对象。
 * @param currentMode 本次操作使用的 `currentMode` 枚举或模式值。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FLRItemUseResult ULRInventoryComponent::UseItemFromSelector(const FName itemId, AActor* target,
	const ELRPerceptionMode currentMode)
{
	return UseItem(BuildUseRequest(itemId, INDEX_NONE, target, currentMode, ELRItemUseEntryPoint::InteractionSelector));
}

/**
 * @brief 根据当前领域状态构建 Build Use Request 所需的数据，不把临时对象作为长期存档标识。
 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
 * @param sourceSlot 本次操作使用的计数、增量或索引 `sourceSlot`；由函数校验合法范围。
 * @param target 本次规则检查或操作的目标对象。
 * @param currentMode 本次操作使用的 `currentMode` 枚举或模式值。
 * @param entryPoint 本次操作使用的 `entryPoint` 枚举或模式值。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FLRItemUseRequest ULRInventoryComponent::BuildUseRequest(const FName itemId, const int32 sourceSlot, UObject* target,
	const ELRPerceptionMode currentMode, const ELRItemUseEntryPoint entryPoint) const
{
	FLRItemUseRequest request;
	request.ItemId = itemId;
	request.SourceSlot = sourceSlot;
	request.Target = target;
	request.Instigator = GetOwner();
	request.EntryPoint = entryPoint;
	request.CurrentMode = currentMode;
	request.ActionTag = LRGameplayTags::InteractionActionUse;
	return request;
}

/**
 * @brief 执行 Resolve Use Request At Time 的纯规则或事务判定，失败时提供结构化原因。
 * @param request 不可变领域请求，包含本次操作所需的稳定 ID、来源、目标或原因。
 * @param currentTimeSeconds 时间值 `currentTimeSeconds`，单位为秒。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FLRItemUseResult ULRInventoryComponent::ResolveUseRequestAtTime(const FLRItemUseRequest& request,
	const double currentTimeSeconds)
{
	return Resolver ? Resolver->ResolveAtTime(request, currentTimeSeconds) : FLRItemUseResult();
}

/**
 * @brief 把稳定笔记 ID 加入已读集合；集合语义保证重复阅读不会产生重复记录。
 * @param noteId 稳定标识 `noteId`；用于内容查询和存档，不依赖显示名或数组序号。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRInventoryComponent::AddNoteId(const FName noteId)
{
	return !noteId.IsNone() && NoteIds.Add(noteId).IsValidId();
}

/**
 * @brief 把稳定收藏品 ID 加入已收集集合；集合语义保证一次性拾取。
 * @param collectibleId 稳定标识 `collectibleId`；用于内容查询和存档，不依赖显示名或数组序号。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRInventoryComponent::AddCollectibleId(const FName collectibleId)
{
	return !collectibleId.IsNone() && CollectibleIds.Add(collectibleId).IsValidId();
}

/**
 * @brief 查询 Owned Item Tags；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FGameplayTagContainer ULRInventoryComponent::GetOwnedItemTags() const
{
	FGameplayTagContainer tags;
	for (const TPair<FName, int32>& item : ItemCounts)
	{
		if (item.Value > 0)
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
	for (const TPair<FName, int32>& item : ItemCounts)
	{
		if (item.Value > 0)
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
 * @brief 把运行时库存、笔记、收藏品或剧情状态复制到存档分块。
 * @param outInventory 参与本次操作的运行时对象 `outInventory`；函数会检查空值和所需接口。
 */
void ULRInventoryComponent::CaptureSaveState(FLRSaveInventoryChunk& outInventory) const
{
	outInventory.ItemCounts = ItemCounts;
	outInventory.QuickSlots = QuickSlots;
	outInventory.SelectedQuickSlot = SelectedQuickSlot;
	outInventory.NoteIds = NoteIds;
	outInventory.CollectibleIds = CollectibleIds;
}

/**
 * @brief 从存档分块恢复运行时状态，并通过领域 API 保持不变量。
 * @param savedInventory 参与本次操作的运行时对象 `savedInventory`；函数会检查空值和所需接口。
 */
void ULRInventoryComponent::RestoreSaveState(const FLRSaveInventoryChunk& savedInventory)
{
	ItemCounts.Reset();
	for (const TPair<FName, int32>& item : savedInventory.ItemCounts)
	{
		if (item.Value > 0 && Definitions.Contains(item.Key))
		{
			ItemCounts.Add(item.Key, item.Value);
		}
	}
	QuickSlots = savedInventory.QuickSlots;
	QuickSlots.SetNum(QuickSlotCount);
	SelectedQuickSlot = FMath::Clamp(savedInventory.SelectedQuickSlot, 0, QuickSlotCount - 1);
	NoteIds = savedInventory.NoteIds;
	CollectibleIds = savedInventory.CollectibleIds;

	for (const TPair<FName, int32>& item : ItemCounts)
	{
		OnInventoryChanged.Broadcast(item.Key, item.Value);
	}
	for (int32 slotIndex = 0; slotIndex < QuickSlotCount; ++slotIndex)
	{
		OnQuickSlotChanged.Broadcast(slotIndex, QuickSlots[slotIndex]);
	}
}

/**
 * @brief 执行 Use Item 的玩法动作；输入层只提供语义，合法性由对应领域组件决定。
 * @param request 不可变领域请求，包含本次操作所需的稳定 ID、来源、目标或原因。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FLRItemUseResult ULRInventoryComponent::UseItem(const FLRItemUseRequest& request)
{
	const double currentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	FLRItemUseResult result = ResolveUseRequestAtTime(request, currentTime);
	OnItemUseResolved.Broadcast(result);
	return result;
}

/**
 * @brief 在事务执行前尝试扣除一个物品；目标执行失败时由调用方恢复。
 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRInventoryComponent::TryConsumeItem(const FName itemId)
{
	int32* count = ItemCounts.Find(itemId);
	if (!count || *count <= 0)
	{
		return false;
	}
	--(*count);
	OnInventoryChanged.Broadcast(itemId, *count);
	return true;
}

/**
 * @brief 把 Restore Item 数据应用到运行时对象，并显式处理缺失依赖。
 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
 */
void ULRInventoryComponent::RestoreItem(const FName itemId)
{
	int32& count = ItemCounts.FindOrAdd(itemId);
	++count;
	OnInventoryChanged.Broadcast(itemId, count);
}
