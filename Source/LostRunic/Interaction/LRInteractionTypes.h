#pragma once

#include "Core/LRTypes.h"
#include "GameplayTagContainer.h"

#include "LRInteractionTypes.generated.h"

/** Distance band for the currently selected interaction candidate. */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Interaction Range"))
enum class ELRInteractionRange : uint8
{
	None UMETA(DisplayName = "None"),
	FarHint UMETA(DisplayName = "Far Hint"),
	Outline UMETA(DisplayName = "Outline"),
	Executable UMETA(DisplayName = "Executable")
};

/** Authored action offered by an interactable target. */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Interaction Option"))
struct LOSTRUNIC_API FLRInteractionOption
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FGameplayTag ActionTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FText Prompt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "0.0", Units = "cm"))
	float MaxDistanceOverride = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	ELRPerceptionMode RequiredMode = ELRPerceptionMode::Normal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FGameplayTagQuery RequiredItemTags;
};

/** Structured interaction execution outcome. */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Interaction Result"))
struct LOSTRUNIC_API FLRInteractionResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	bool bSuccess = false;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	FGameplayTag ActionTag;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	FGameplayTag FailureReason;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	FName EventId = NAME_None;
};

/** Pure score data used by the candidate filter and automation tests. */
struct LOSTRUNIC_API FLRInteractionCandidateScore
{
	float Distance = 0.0f;
	float ForwardDot = 1.0f;
	bool bOccluded = false;
	bool bModeAllowed = true;
	bool bItemsAllowed = true;
};
