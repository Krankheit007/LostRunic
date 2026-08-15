/** @file LRSaveTypes.h @brief V2 shared save identifiers and resume contracts. */
#pragma once

#include "Core/LRTypes.h"

#include "LRSaveTypes.generated.h"

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

UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Save Slot Type"))
enum class ELRSaveSlotType : uint8
{
	Auto UMETA(DisplayName = "Auto"),
	Manual UMETA(DisplayName = "Manual")
};

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

namespace LRSaveIds
{
	LOSTRUNIC_API extern const FName AutoSlotReason;
	LOSTRUNIC_API extern const FName MemoryEntryReason;
	LOSTRUNIC_API extern const FName MemoryReturnReason;
	LOSTRUNIC_API extern const FName MemoryMapId;
}
