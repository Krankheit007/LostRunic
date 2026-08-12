/**
 * @file LRMenuWidgetController.cpp
 * @brief 统一菜单控制器：维护背包/笔记/收集品/暂停/存档槽 Tab 状态，构建不含快捷栏数据的库存快照；交互选物模式下计算物品与目标兼容性。
 *
 * 关联文件：LRMenuWidgetController.h；所属领域：UI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "UI/LRMenuWidgetController.h"

#include "Core/LRGameplayTags.h"
#include "Data/LRItemDefinition.h"
#include "GameFramework/Actor.h"
#include "Items/LRInventoryComponent.h"
#include "Items/LRItemUseTarget.h"

/**
 * @brief 切换到指定菜单页面并广播变化；同一时刻只保留一个可见菜单。
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
	snapshot.SelectedWeaponItemId = inventory->GetSelectedWeapon();
	snapshot.EffectiveWeaponItemId = inventory->GetEffectiveWeapon();

	FGameplayTagContainer targetTags;
	if (itemUseTarget && itemUseTarget->GetClass()->ImplementsInterface(ULRItemUseTarget::StaticClass()))
	{
		targetTags = ILRItemUseTarget::Execute_GetItemUseTargetTags(itemUseTarget);
	}

	for (const FName itemId : inventory->GetOwnedItemIds())
	{
		const ULRItemDefinition* definition = inventory->FindDefinition(itemId);
		if (!definition)
		{
			continue;
		}
		const FLRInventoryEntry* entry = inventory->FindEntry(itemId);
		FLRInventoryItemView& view = snapshot.Items.AddDefaulted_GetRef();
		view.ItemId = itemId;
		view.DisplayName = definition->DisplayName.IsEmpty() ? FText::FromName(itemId) : definition->DisplayName;
		view.Description = definition->Description;
		view.Quantity = entry ? entry->Quantity : 0;
		view.bConsumable = definition->bConsumable;
		view.bIsWeapon = definition->ItemTags.HasTag(LRGameplayTags::ItemCategoryWeapon);
		view.bIsSelectedWeapon = view.bIsWeapon && snapshot.SelectedWeaponItemId == itemId;
		if (targetTags.IsValid())
		{
			view.bCompatibleWithTarget = definition->AllowedActionTags.HasTag(LRGameplayTags::InteractionActionUse)
				&& targetTags.HasAny(definition->AllowedTargetTags);
			view.FailureReason = view.bCompatibleWithTarget ? FGameplayTag() : LRGameplayTags::InteractionRejectItem;
		}
	}
	return snapshot;
}
