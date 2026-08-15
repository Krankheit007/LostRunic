/** @file LRSaveCatalog.h @brief A/B metadata catalog for V2 slots. */
#pragma once

#include "GameFramework/SaveGame.h"
#include "Save/LRSaveV2Types.h"

#include "LRSaveCatalog.generated.h"

UCLASS()
class LOSTRUNIC_API ULRSaveCatalog : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr int32 LatestVersion = 1;
	bool Validate(FString& outError) const;
	const FLRSaveSlotMetadata* FindSlot(const FLRSaveSlotId& slotId) const;
	FLRSaveSlotMetadata* FindSlot(const FLRSaveSlotId& slotId);
	int32 FindLowestFreeDisplayIndex(int32 maxManualSlots) const;
	void SortSlots();

	UPROPERTY(SaveGame) int32 CatalogVersion = LatestVersion;
	UPROPERTY(SaveGame) int64 Generation = 0;
	UPROPERTY(SaveGame) TArray<FLRSaveSlotMetadata> Slots;
	UPROPERTY(SaveGame) FLRCatalogPendingOperation PendingOperation;
};

namespace LRSaveCatalogNames
{
	LOSTRUNIC_API const FString& A();
	LOSTRUNIC_API const FString& B();
}
