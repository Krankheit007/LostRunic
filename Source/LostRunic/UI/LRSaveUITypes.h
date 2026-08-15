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

UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Save Focus Target"))
enum class ELRSaveFocusTargetKind : uint8
{
	None,
	Root,
	ExistingSlot,
	CreateSlot
};

USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Save Focus Target"))
struct LOSTRUNIC_API FLRSaveFocusTarget
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Save UI|Focus")
	ELRSaveFocusTargetKind Kind = ELRSaveFocusTargetKind::None;

	UPROPERTY(BlueprintReadOnly, Category = "Save UI|Focus")
	FLRSaveSlotId SlotId;

	UPROPERTY(BlueprintReadOnly, Category = "Save UI|Focus")
	int32 CreateDisplayIndex = 0;

	static FLRSaveFocusTarget MakeRoot()
	{
		FLRSaveFocusTarget target;
		target.Kind = ELRSaveFocusTargetKind::Root;
		return target;
	}

	static FLRSaveFocusTarget MakeExisting(const FLRSaveSlotId& slotId)
	{
		FLRSaveFocusTarget target;
		target.Kind = ELRSaveFocusTargetKind::ExistingSlot;
		target.SlotId = slotId;
		return target;
	}

	static FLRSaveFocusTarget MakeCreate(const int32 displayIndex)
	{
		FLRSaveFocusTarget target;
		target.Kind = ELRSaveFocusTargetKind::CreateSlot;
		target.CreateDisplayIndex = displayIndex;
		return target;
	}

	bool IsValid() const
	{
		return Kind == ELRSaveFocusTargetKind::Root
			|| (Kind == ELRSaveFocusTargetKind::ExistingSlot && SlotId.IsValid())
			|| (Kind == ELRSaveFocusTargetKind::CreateSlot && CreateDisplayIndex > 0);
	}
};

USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Save Confirmation View Model"))
struct LOSTRUNIC_API FLRSaveConfirmViewModel
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Save UI|Confirmation")
	ELRSaveUIConfirmation Confirmation = ELRSaveUIConfirmation::None;

	UPROPERTY(BlueprintReadOnly, Category = "Save UI|Confirmation")
	FLRSaveSlotId SlotId;

	UPROPERTY(BlueprintReadOnly, Category = "Save UI|Confirmation")
	FText Message;

	UPROPERTY(BlueprintReadOnly, Category = "Save UI|Confirmation")
	bool bVisible = false;
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
	UPROPERTY(BlueprintReadOnly, Category = "Save UI") int32 CollectedCount = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Save UI") int32 TotalCollectibleCount = 0;
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
	UPROPERTY(BlueprintReadOnly, Category = "Save UI") FLRSaveConfirmViewModel ConfirmationViewModel;
	UPROPERTY(BlueprintReadOnly, Category = "Save UI") TArray<FLRSaveSlotView> Slots;
	UPROPERTY(BlueprintReadOnly, Category = "Save UI") FLRSaveSlotId SelectedSlotId;
	UPROPERTY(BlueprintReadOnly, Category = "Save UI|Focus") FLRSaveFocusTarget FocusTarget;
	UPROPERTY(BlueprintReadOnly, Category = "Save UI|Focus") int32 CreateDisplayIndex = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Save UI") FText StatusMessage;
	UPROPERTY(BlueprintReadOnly, Category = "Save UI") bool bCanCreateManualSlot = false;
	UPROPERTY(BlueprintReadOnly, Category = "Save UI") bool bIsBusy = false;
};
