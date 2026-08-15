/** @file LRSavePayload.h @brief Immutable V2 slot payload. */
#pragma once

#include "GameFramework/SaveGame.h"
#include "Save/LRSaveV2Types.h"

#include "LRSavePayload.generated.h"

UCLASS(BlueprintType)
class LOSTRUNIC_API ULRSavePayload : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr int32 LatestVersion = 2;
	bool ValidatePayload(const FLRSaveSlotMetadata* expectedMetadata, ELRSaveSlotHealth& outHealth,
		FString& outError) const;

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save") int32 SaveVersion = LatestVersion;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save") FLRSaveSlotId SlotId;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save") FString PayloadKey;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save") int64 SaveSequence = 0;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save") FLRSaveSlotMetadata MetadataSnapshot;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save") FLRSaveDataV2 Data;
};
