/**
 * @file LRMenuWidgetController.cpp
 * @brief 实现 HUD、状态遮罩、对话/阅读、背包/笔记/收藏、暂停、存档槽和过场的控制器边界。UI 订阅领域事件并负责表现，不参与核心规则判定。
 *
 * 关联文件：LRMenuWidgetController.h；所属领域：UI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "UI/LRMenuWidgetController.h"

#include "Items/LRInventoryComponent.h"

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
 * @brief 根据当前领域状态构建 Build Inventory Snapshot 所需的数据，不把临时对象作为长期存档标识。
 * @param inventory 参与本次操作的运行时对象 `inventory`；函数会检查空值和所需接口。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FLRInventorySnapshot ULRMenuWidgetController::BuildInventorySnapshot(const ULRInventoryComponent* inventory) const
{
	FLRInventorySnapshot snapshot;
	if (!inventory)
	{
		return snapshot;
	}
	snapshot.ItemIds = inventory->GetOwnedItemIds();
	snapshot.NoteIds = inventory->GetNoteIds();
	snapshot.CollectibleIds = inventory->GetCollectibleIds();
	snapshot.SelectedQuickSlot = inventory->GetSelectedQuickSlot();
	for (int32 slotIndex = 0; slotIndex < 4; ++slotIndex)
	{
		snapshot.QuickSlots.Add(inventory->GetQuickSlotItem(slotIndex));
	}
	return snapshot;
}
