#include "UI/LRMenuWidgetController.h"

#include "Items/LRInventoryComponent.h"

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
