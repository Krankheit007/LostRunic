#pragma once

#include "Core/LRTypes.h"
#include "GameplayTagContainer.h"

#include "LRStateTypes.generated.h"

/** Semantic origin of a perception state request. */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic State Request Type"))
enum class ELRStateRequestType : uint8
{
	None UMETA(DisplayName = "None"),
	CloseEyes UMETA(DisplayName = "Close Eyes"),
	OpenEyes UMETA(DisplayName = "Open Eyes"),
	Death UMETA(DisplayName = "Death"),
	Narrative UMETA(DisplayName = "Narrative")
};

/** Immutable intent submitted to the state rule owner. */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic State Change Request"))
struct LOSTRUNIC_API FLRStateChangeRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	ELRPerceptionMode TargetMode = ELRPerceptionMode::Normal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	ELRStateRequestType RequestType = ELRStateRequestType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	FGameplayTag Source;
};

/** Structured outcome returned for every state request. */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic State Change Result"))
struct LOSTRUNIC_API FLRStateChangeResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bAccepted = false;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	ELRPerceptionMode PreviousMode = ELRPerceptionMode::Normal;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	ELRPerceptionMode CurrentMode = ELRPerceptionMode::Normal;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	FGameplayTag Reason;
};
