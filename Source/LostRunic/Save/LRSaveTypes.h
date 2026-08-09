#pragma once

#include "Core/LRTypes.h"

#include "LRSaveTypes.generated.h"

class ULRSaveGame;

/** Stable destination used when a resumed game must return to its last safe location. */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Resume Anchor"))
struct LOSTRUNIC_API FLRResumeAnchor
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Resume")
	FName MapId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Resume")
	FName AnchorId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Resume")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Resume")
	FRotator Rotation = FRotator::ZeroRotator;

	bool IsValid() const { return !MapId.IsNone() && !AnchorId.IsNone(); }
};

/** Inventory values copied to disk without serializing runtime components or definitions. */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Save Inventory"))
struct LOSTRUNIC_API FLRSaveInventoryChunk
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Inventory")
	TMap<FName, int32> ItemCounts;

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Inventory")
	TArray<FName> QuickSlots;

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Inventory")
	int32 SelectedQuickSlot = 0;

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Inventory")
	TSet<FName> NoteIds;

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Inventory")
	TSet<FName> CollectibleIds;
};

/** Narrative, memory, and death progress that must survive level travel. */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Save Narrative"))
struct LOSTRUNIC_API FLRSaveNarrativeChunk
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Narrative")
	TSet<FName> CompletedEventIds;

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Narrative")
	TSet<FName> MemoryEventIds;

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Narrative")
	int32 DeathCount = 0;
};

/** Save destination selection for one automatic slot or a bounded manual slot. */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Save Slot Type"))
enum class ELRSaveSlotType : uint8
{
	Auto UMETA(DisplayName = "Auto"),
	Manual UMETA(DisplayName = "Manual")
};

/** Request category controls debouncing and the Memory transaction step. */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Save Write Kind"))
enum class ELRSaveWriteKind : uint8
{
	Auto UMETA(DisplayName = "Auto"),
	Manual UMETA(DisplayName = "Manual"),
	Critical UMETA(DisplayName = "Critical"),
	MemoryEntry UMETA(DisplayName = "Memory Entry"),
	MemoryEvent UMETA(DisplayName = "Memory Event"),
	MemoryReturn UMETA(DisplayName = "Memory Return")
};

/** Structured response to all save/load requests. */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Save Request Result"))
enum class ELRSaveRequestResult : uint8
{
	Queued UMETA(DisplayName = "Queued"),
	Scheduled UMETA(DisplayName = "Scheduled"),
	Loaded UMETA(DisplayName = "Loaded"),
	RejectedInvalidSlot UMETA(DisplayName = "Rejected Invalid Slot"),
	RejectedMemoryManual UMETA(DisplayName = "Rejected In Memory"),
	RejectedInvalidAnchor UMETA(DisplayName = "Rejected Invalid Anchor"),
	RejectedUnavailableMap UMETA(DisplayName = "Rejected Unavailable Map"),
	MissingOrCorrupt UMETA(DisplayName = "Missing Or Corrupt")
};

/** Explicit state machine for the death-to-memory persistence transaction. */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Memory Transaction Phase"))
enum class ELRMemoryTransactionPhase : uint8
{
	None UMETA(DisplayName = "None"),
	AwaitingMemoryWorld UMETA(DisplayName = "Awaiting Memory World"),
	SavingEntry UMETA(DisplayName = "Saving Entry"),
	InMemory UMETA(DisplayName = "In Memory"),
	AwaitingResumeWorld UMETA(DisplayName = "Awaiting Resume World"),
	SavingReturn UMETA(DisplayName = "Saving Return")
};

/** Immutable request record retained by the game-thread FIFO until its async write completes. */
USTRUCT(meta = (DisplayName = "Lost Runic Queued Save Request"))
struct LOSTRUNIC_API FLRQueuedSaveRequest
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<ULRSaveGame> Snapshot;

	FString SlotName;
	FName ReasonId = NAME_None;
	ELRSaveWriteKind Kind = ELRSaveWriteKind::Auto;
	int32 RetryAttempt = 0;
};

namespace LRSaveIds
{
	LOSTRUNIC_API extern const FName AutoSlotReason;
	LOSTRUNIC_API extern const FName MemoryEntryReason;
	LOSTRUNIC_API extern const FName MemoryReturnReason;
	LOSTRUNIC_API extern const FName MemoryMapId;
}
