/** @file LRSaveUITypes.h @brief Read-only presentation contracts for the V2 save UI. */
#pragma once

#include "CoreMinimal.h"
#include "Save/LRSaveV2Types.h"

#include "LRSaveUITypes.generated.h"

UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Save Confirmation"))
enum class ELRSaveUIConfirmation : uint8
{
	None,
	Overwrite,
	Delete
};

USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Save Slot View"))
struct LOSTRUNIC_API FLRSaveSlotView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Save UI") FLRSaveSlotId SlotId;
	UPROPERTY(BlueprintReadOnly, Category = "Save UI") int32 DisplayIndex = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Save UI") FText MapDisplayName;
	UPROPERTY(BlueprintReadOnly, Category = "Save UI") FDateTime SavedAtUtc;
	UPROPERTY(BlueprintReadOnly, Category = "Save UI") double PlayTimeSeconds = 0.0;
	UPROPERTY(BlueprintReadOnly, Category = "Save UI") ELRSaveSlotHealth Health = ELRSaveSlotHealth::Healthy;
	UPROPERTY(BlueprintReadOnly, Category = "Save UI") bool bAutomatic = false;
	UPROPERTY(BlueprintReadOnly, Category = "Save UI") bool bCanLoad = false;
	UPROPERTY(BlueprintReadOnly, Category = "Save UI") bool bCanOverwrite = false;
	UPROPERTY(BlueprintReadOnly, Category = "Save UI") bool bCanDelete = false;
};

USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Save UI Snapshot"))
struct LOSTRUNIC_API FLRSaveUISnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Save UI") ELRSaveSelectionMode Mode = ELRSaveSelectionMode::Load;
	UPROPERTY(BlueprintReadOnly, Category = "Save UI") ELRSaveUIState State = ELRSaveUIState::Idle;
	UPROPERTY(BlueprintReadOnly, Category = "Save UI") ELRSaveUIConfirmation Confirmation = ELRSaveUIConfirmation::None;
	UPROPERTY(BlueprintReadOnly, Category = "Save UI") TArray<FLRSaveSlotView> Slots;
	UPROPERTY(BlueprintReadOnly, Category = "Save UI") FLRSaveSlotId SelectedSlotId;
	UPROPERTY(BlueprintReadOnly, Category = "Save UI") FText StatusMessage;
	UPROPERTY(BlueprintReadOnly, Category = "Save UI") bool bCanCreateManualSlot = false;
	UPROPERTY(BlueprintReadOnly, Category = "Save UI") bool bIsBusy = false;
};
