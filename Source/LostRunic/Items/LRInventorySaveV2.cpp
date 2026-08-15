/** @file LRInventorySaveV2.cpp @brief V2 save provider adapters for inventory-owned runtime data. */
#include "Items/LRInventoryComponent.h"

void ULRInventoryComponent::CaptureInventorySaveState(FLRSaveInventoryChunkV2& outInventory) const
{
	outInventory.ItemCounts.Reset();
	outInventory.AcquisitionSequences.Reset();
	for (const TPair<FName, FLRInventoryEntry>& item : Entries)
	{
		if (item.Value.Quantity > 0)
		{
			outInventory.ItemCounts.Add(item.Key, item.Value.Quantity);
			outInventory.AcquisitionSequences.Add(item.Key, item.Value.AcquisitionSequence);
		}
	}
	outInventory.SelectedWeaponItemId = GetSelectedWeapon();
}

void ULRInventoryComponent::RestoreInventorySaveState(const FLRSaveInventoryChunkV2& savedInventory)
{
	Entries.Reset();
	SelectedWeaponItemId = NAME_None;
	NextAcquisitionSequence = 1;
	for (const TPair<FName, int32>& item : savedInventory.ItemCounts)
	{
		if (item.Value <= 0 || !Definitions.Contains(item.Key))
		{
			continue;
		}
		FLRInventoryEntry& entry = Entries.Add(item.Key);
		entry.ItemId = item.Key;
		entry.Quantity = item.Value;
		entry.AcquisitionSequence = savedInventory.AcquisitionSequences.FindRef(item.Key);
		if (entry.AcquisitionSequence <= 0)
		{
			entry.AcquisitionSequence = NextAcquisitionSequence;
		}
		NextAcquisitionSequence = FMath::Max(NextAcquisitionSequence, entry.AcquisitionSequence + 1);
		OnInventoryChanged.Broadcast(item.Key, item.Value);
	}
	SetSelectedWeapon(savedInventory.SelectedWeaponItemId);
}

void ULRInventoryComponent::CaptureNotebookSaveState(FLRSaveNotebookChunk& outNotebook) const
{
	outNotebook.NoteIds = NoteIds;
}

void ULRInventoryComponent::RestoreNotebookSaveState(const FLRSaveNotebookChunk& savedNotebook)
{
	NoteIds = savedNotebook.NoteIds;
	OnNotesChanged.Broadcast();
}

void ULRInventoryComponent::CaptureCollectibleSaveState(FLRSaveCollectibleChunk& outCollectible) const
{
	outCollectible.CollectibleIds = CollectibleIds;
}

void ULRInventoryComponent::RestoreCollectibleSaveState(const FLRSaveCollectibleChunk& savedCollectible)
{
	CollectibleIds = savedCollectible.CollectibleIds;
	OnCollectiblesChanged.Broadcast();
}
