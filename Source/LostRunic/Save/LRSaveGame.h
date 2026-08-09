#pragma once

#include "GameFramework/SaveGame.h"
#include "Save/LRSaveTypes.h"

#include "LRSaveGame.generated.h"

/** Disk representation of Lost Runic progress. Runtime references never enter this object. */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Save Game"))
class LOSTRUNIC_API ULRSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr int32 LatestVersion = 1;

	bool MigrateToLatest(FString& outError);

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save")
	int32 SaveVersion = LatestVersion;

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save")
	FDateTime LastSavedUtc;

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Resume")
	FLRResumeAnchor ResumeAnchor;

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Progress")
	FLRSaveInventoryChunk Inventory;

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Progress")
	FLRSaveNarrativeChunk Narrative;

	UPROPERTY(SaveGame, meta = (DeprecatedProperty, DeprecationMessage = "Migrated into ResumeAnchor in save version 1."))
	FName LegacyMapId = NAME_None;

	UPROPERTY(SaveGame, meta = (DeprecatedProperty, DeprecationMessage = "Migrated into ResumeAnchor in save version 1."))
	FVector LegacyLocation = FVector::ZeroVector;

	UPROPERTY(SaveGame, meta = (DeprecatedProperty, DeprecationMessage = "Migrated into ResumeAnchor in save version 1."))
	FRotator LegacyRotation = FRotator::ZeroRotator;
};
