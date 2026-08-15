#include "Save/LRSaveCatalog.h"

bool ULRSaveCatalog::Validate(FString& outError) const
{
	if (CatalogVersion != LatestVersion || Generation < 0)
	{
		outError = TEXT("Catalog version or generation is invalid.");
		return false;
	}
	TSet<FLRSaveSlotId> slotIds;
	TSet<int32> displayIndices;
	int32 autoSlots = 0;
	for (const FLRSaveSlotMetadata& slot : Slots)
	{
		if (!slot.SlotId.IsValid() || slot.PayloadKey.IsEmpty() || slot.SaveSequence <= 0 || slotIds.Contains(slot.SlotId))
		{
			outError = TEXT("Catalog contains an invalid or duplicate slot identity.");
			return false;
		}
		slotIds.Add(slot.SlotId);
		if (slot.SlotId.Type == ELRSaveSlotType::Auto)
		{
			++autoSlots;
			if (slot.SlotId.Guid != LRSaveV2Ids::AutoSlotGuid)
			{
				outError = TEXT("Catalog automatic slot uses an unknown identity.");
				return false;
			}
		}
		else if (slot.DisplayIndex <= 0 || displayIndices.Contains(slot.DisplayIndex))
		{
			outError = TEXT("Catalog contains a duplicate or invalid manual display index.");
			return false;
		}
		else
		{
			displayIndices.Add(slot.DisplayIndex);
		}
	}
	if (autoSlots > 1 || (PendingOperation.IsSet() && !PendingOperation.TargetMetadata.SlotId.IsValid()))
	{
		outError = TEXT("Catalog automatic slot or pending operation is invalid.");
		return false;
	}
	return true;
}

const FLRSaveSlotMetadata* ULRSaveCatalog::FindSlot(const FLRSaveSlotId& slotId) const
{
	return Slots.FindByPredicate([&slotId](const FLRSaveSlotMetadata& slot) { return slot.SlotId == slotId; });
}

FLRSaveSlotMetadata* ULRSaveCatalog::FindSlot(const FLRSaveSlotId& slotId)
{
	return Slots.FindByPredicate([&slotId](const FLRSaveSlotMetadata& slot) { return slot.SlotId == slotId; });
}

int32 ULRSaveCatalog::FindLowestFreeDisplayIndex(const int32 maxManualSlots) const
{
	for (int32 candidate = 1; candidate <= maxManualSlots; ++candidate)
	{
		if (!Slots.ContainsByPredicate([candidate](const FLRSaveSlotMetadata& slot)
			{ return slot.SlotId.Type == ELRSaveSlotType::Manual && slot.DisplayIndex == candidate; }))
		{
			return candidate;
		}
	}
	return INDEX_NONE;
}

void ULRSaveCatalog::SortSlots()
{
	Slots.Sort([](const FLRSaveSlotMetadata& a, const FLRSaveSlotMetadata& b)
	{
		return a.SlotId.Type != b.SlotId.Type ? a.SlotId.Type == ELRSaveSlotType::Auto : a.DisplayIndex < b.DisplayIndex;
	});
}

namespace LRSaveCatalogNames
{
	const FString& A() { static const FString name(TEXT("LostRunic_V2_Catalog_A")); return name; }
	const FString& B() { static const FString name(TEXT("LostRunic_V2_Catalog_B")); return name; }
}
