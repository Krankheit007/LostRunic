#pragma once

#include "Core/LRTypes.h"
#include "GameplayTagContainer.h"

#include "LRItemUseTypes.generated.h"

class AActor;

/** UI path that created an otherwise identical item-use transaction. */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Item Use Entry Point"))
enum class ELRItemUseEntryPoint : uint8
{
	QuickSlot UMETA(DisplayName = "Quick Slot"),
	InteractionSelector UMETA(DisplayName = "Interaction Selector")
};

/** Complete immutable request consumed by ULRItemUseResolver. */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Item Use Request"))
struct LOSTRUNIC_API FLRItemUseRequest
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Item Use")
	FName ItemId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Item Use")
	int32 SourceSlot = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Item Use")
	TObjectPtr<UObject> Target;

	UPROPERTY(BlueprintReadOnly, Category = "Item Use")
	TObjectPtr<AActor> Instigator;

	UPROPERTY(BlueprintReadOnly, Category = "Item Use")
	ELRItemUseEntryPoint EntryPoint = ELRItemUseEntryPoint::QuickSlot;

	UPROPERTY(BlueprintReadOnly, Category = "Item Use")
	ELRPerceptionMode CurrentMode = ELRPerceptionMode::Normal;

	UPROPERTY(BlueprintReadOnly, Category = "Item Use")
	FGameplayTag ActionTag;
};

/** Transaction result shared by quick-slot and selector entry points. */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Item Use Result"))
struct LOSTRUNIC_API FLRItemUseResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Item Use")
	bool bSuccess = false;

	UPROPERTY(BlueprintReadOnly, Category = "Item Use")
	bool bConsumed = false;

	UPROPERTY(BlueprintReadOnly, Category = "Item Use")
	FName ItemId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Item Use")
	FName EventId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Item Use")
	FGameplayTag FailureReason;
};
