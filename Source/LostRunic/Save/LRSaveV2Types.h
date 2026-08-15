/** @file LRSaveV2Types.h @brief V2 slot, catalog, operation and chunk contracts. */
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Save/LRSaveTypes.h"

#include "LRSaveV2Types.generated.h"

class ULRSavePayload;

UENUM(BlueprintType)
enum class ELRSaveSlotHealth : uint8
{
	Healthy, MissingPayload, CorruptPayload, UnsupportedVersion, CatalogMismatch, UnknownSlotType, InvalidData
};

UENUM(BlueprintType)
enum class ELRSaveOperationType : uint8
{
	None, CreateManual, OverwriteManual, AutoSave, CriticalSave, Load, Delete, Continue, NewGame, RepairHealth
};

UENUM(BlueprintType)
enum class ELRSaveOperationState : uint8
{
	Idle, RecoveringCatalog, Capturing, WritingPayload, CommittingCatalog, ReadingPayload, AwaitingWorld,
	Restoring, DeletingPayload, RepairingHealth
};

UENUM(BlueprintType)
enum class ELRSaveMemoryPurpose : uint8
{
	None, Entry, Event, Return
};

UENUM(BlueprintType)
enum class ELRSaveResultCode : uint8
{
	Queued, Succeeded, RejectedBusy, RejectedInvalidSlot, RejectedNotEligible, RejectedAtCapacity,
	RejectedProtectedSlot, ProviderUnavailable, InvalidData, MissingPayload, UnsupportedVersion, CatalogMismatch,
	ReadFailed, WriteFailed, DeleteFailed, TimedOut, Cancelled
};

UENUM(BlueprintType)
enum class ELRCatalogPendingType : uint8 { None, Write, Delete };

UENUM(BlueprintType)
enum class ELRSaveUIState : uint8 { Idle, LoadingCatalog, Confirming, Saving, Loading, Deleting, Error };

UENUM(BlueprintType)
enum class ELRSaveSelectionMode : uint8 { Save, Load };

USTRUCT(BlueprintType)
struct LOSTRUNIC_API FLRSaveSlotId
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save|Slot")
	ELRSaveSlotType Type = ELRSaveSlotType::Manual;

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save|Slot")
	FGuid Guid;

	bool IsValid() const { return Guid.IsValid(); }
	bool operator==(const FLRSaveSlotId& other) const { return Type == other.Type && Guid == other.Guid; }
	bool operator!=(const FLRSaveSlotId& other) const { return !(*this == other); }
};

FORCEINLINE uint32 GetTypeHash(const FLRSaveSlotId& slotId)
{
	return HashCombine(GetTypeHash(slotId.Guid), GetTypeHash(static_cast<uint8>(slotId.Type)));
}

USTRUCT(BlueprintType)
struct LOSTRUNIC_API FLRSaveSlotMetadata
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save|Slot") FLRSaveSlotId SlotId;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save|Slot") int32 DisplayIndex = 0;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save|Slot") FString PayloadKey;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save|Slot") FName MapId = NAME_None;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save|Slot") FDateTime SavedAtUtc;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save|Slot") double PlayTimeSeconds = 0.0;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save|Slot") int64 SaveSequence = 0;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save|Slot") ELRSaveSlotHealth Health = ELRSaveSlotHealth::Healthy;
};

USTRUCT()
struct LOSTRUNIC_API FLRCatalogPendingOperation
{
	GENERATED_BODY()
	UPROPERTY(SaveGame) ELRCatalogPendingType Type = ELRCatalogPendingType::None;
	UPROPERTY(SaveGame) FLRSaveSlotMetadata PreviousMetadata;
	UPROPERTY(SaveGame) FLRSaveSlotMetadata TargetMetadata;
	bool IsSet() const { return Type != ELRCatalogPendingType::None; }
};

USTRUCT(BlueprintType)
struct LOSTRUNIC_API FLRSavePlayerChunkV2
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save|Player") FLRResumeAnchor ResumeAnchor;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save|Player") FName CurrentMapId = NAME_None;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save|Player") FVector Location = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save|Player") FRotator Rotation = FRotator::ZeroRotator;
};

USTRUCT(BlueprintType)
struct LOSTRUNIC_API FLRSaveInventoryChunkV2
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save|Inventory") TMap<FName, int32> ItemCounts;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save|Inventory") TMap<FName, int64> AcquisitionSequences;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save|Inventory") FName SelectedWeaponItemId = NAME_None;
};

USTRUCT(BlueprintType)
struct LOSTRUNIC_API FLRSaveNotebookChunk
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save|Notebook") TSet<FName> NoteIds;
};

USTRUCT(BlueprintType)
struct LOSTRUNIC_API FLRSaveCollectibleChunk
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save|Collectible") TSet<FName> CollectibleIds;
};

USTRUCT(BlueprintType)
struct LOSTRUNIC_API FLRSaveStoryChunk
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save|Story") TSet<FName> CompletedEventIds;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save|Story") TSet<FName> MemoryEventIds;
};

USTRUCT(BlueprintType)
struct LOSTRUNIC_API FLRSaveStatisticsChunk
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save|Statistics") int32 DeathCount = 0;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save|Statistics") double PlayTimeSeconds = 0.0;
};

USTRUCT(BlueprintType)
struct LOSTRUNIC_API FLRSaveDataV2
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save") FLRSavePlayerChunkV2 Player;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save") FLRSaveInventoryChunkV2 Inventory;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save") FLRSaveNotebookChunk Notebook;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save") FLRSaveCollectibleChunk Collectible;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save") FLRSaveStoryChunk Story;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save") FLRSaveStatisticsChunk Statistics;
};

USTRUCT(BlueprintType)
struct LOSTRUNIC_API FLRSaveOperationResult
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category = "Save") FGuid OperationId;
	UPROPERTY(BlueprintReadOnly, Category = "Save") ELRSaveOperationType Operation = ELRSaveOperationType::None;
	UPROPERTY(BlueprintReadOnly, Category = "Save") ELRSaveResultCode Code = ELRSaveResultCode::RejectedBusy;
	UPROPERTY(BlueprintReadOnly, Category = "Save") FLRSaveSlotId SlotId;
	UPROPERTY(BlueprintReadOnly, Category = "Save") FGameplayTag FailureReason;
	UPROPERTY(BlueprintReadOnly, Category = "Save") FString Diagnostic;
	bool IsSuccess() const { return Code == ELRSaveResultCode::Succeeded || Code == ELRSaveResultCode::Queued; }
};

USTRUCT()
struct LOSTRUNIC_API FLRQueuedSaveOperation
{
	GENERATED_BODY()
	UPROPERTY(Transient) FGuid OperationId;
	UPROPERTY(Transient) ELRSaveOperationType Type = ELRSaveOperationType::None;
	UPROPERTY(Transient) FLRSaveSlotId SlotId;
	UPROPERTY(Transient) FName ReasonId = NAME_None;
	UPROPERTY(Transient) FLRSaveDataV2 CapturedData;
	UPROPERTY(Transient) ELRSaveMemoryPurpose MemoryPurpose = ELRSaveMemoryPurpose::None;
	UPROPERTY(Transient) ELRSaveSlotHealth RequestedHealth = ELRSaveSlotHealth::Healthy;
	int32 RetryCount = 0;
	int64 CatalogSequence = 0;
	FString PayloadKey;
	bool bHasCapturedData = false;
};

namespace LRSaveV2Ids
{
	LOSTRUNIC_API extern const FGuid AutoSlotGuid;
}
