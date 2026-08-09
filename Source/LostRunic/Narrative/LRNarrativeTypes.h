#pragma once

#include "Core/LRTypes.h"
#include "GameplayTagContainer.h"

#include "LRNarrativeTypes.generated.h"

class UTexture2D;

/** Presentation mode shared by dialogue and readable content. */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Narrative Session Type"))
enum class ELRNarrativeSessionType : uint8
{
	None UMETA(DisplayName = "None"),
	Dialogue UMETA(DisplayName = "Dialogue"),
	Reading UMETA(DisplayName = "Reading")
};

/** Observable outcome of a narrative input transaction. */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Narrative Action"))
enum class ELRNarrativeAction : uint8
{
	Rejected UMETA(DisplayName = "Rejected"),
	Started UMETA(DisplayName = "Started"),
	RevealCurrentText UMETA(DisplayName = "Reveal Current Text"),
	Advanced UMETA(DisplayName = "Advanced"),
	AwaitChoice UMETA(DisplayName = "Await Choice"),
	Completed UMETA(DisplayName = "Completed")
};

/** One currently legal choice prepared for presentation. */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Narrative Choice"))
struct LOSTRUNIC_API FLRNarrativeChoice
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Narrative")
	FName ChoiceId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative")
	FText Text;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative")
	FName NextContentId = NAME_None;
};

/** Immutable page sent by the narrative rules to its UI controller. */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Narrative Page"))
struct LOSTRUNIC_API FLRNarrativePage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Narrative")
	ELRNarrativeSessionType SessionType = ELRNarrativeSessionType::None;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative")
	FName ContentId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative")
	FName SpeakerId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative")
	FText Title;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative")
	FText Text;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative")
	TSoftObjectPtr<UTexture2D> Portrait;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative")
	TArray<FLRNarrativeChoice> Choices;
};

/** Structured result returned by every narrative action. */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Narrative Result"))
struct LOSTRUNIC_API FLRNarrativeResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Narrative")
	bool bSuccess = false;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative")
	ELRNarrativeAction Action = ELRNarrativeAction::Rejected;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative")
	FName ContentId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative")
	FGameplayTag FailureReason;
};
