#pragma once

#include "Core/LRTypes.h"
#include "Narrative/LRNarrativeTypes.h"

#include "LRUITypes.generated.h"

/** Independently authored screen surfaces used by the Lost Runic HUD. */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Screen Type"))
enum class ELRScreenType : uint8
{
	None UMETA(DisplayName = "None"),
	HUD UMETA(DisplayName = "HUD"),
	StateOverlay UMETA(DisplayName = "State Overlay"),
	Narrative UMETA(DisplayName = "Narrative"),
	Journal UMETA(DisplayName = "Journal"),
	Inventory UMETA(DisplayName = "Inventory"),
	Collectibles UMETA(DisplayName = "Collectibles"),
	Pause UMETA(DisplayName = "Pause"),
	SaveSlots UMETA(DisplayName = "Save Slots"),
	Transition UMETA(DisplayName = "Transition")
};

/** Typewriter presentation snapshot. Rules own the page; UI owns only revealed text. */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Narrative Presentation"))
struct LOSTRUNIC_API FLRNarrativePresentation
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Narrative")
	FLRNarrativePage Page;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative")
	FText DisplayedText;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative")
	bool bTextFullyRevealed = false;
};

/** Read-only inventory/journal data supplied to menu layouts. */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Inventory Snapshot"))
struct LOSTRUNIC_API FLRInventorySnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TArray<FName> ItemIds;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TArray<FName> NoteIds;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TArray<FName> CollectibleIds;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TArray<FName> QuickSlots;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 SelectedQuickSlot = 0;
};
