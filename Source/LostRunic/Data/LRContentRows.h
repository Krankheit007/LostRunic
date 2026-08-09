#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"

#include "LRContentRows.generated.h"

class UTexture2D;
class UWorld;

/** A conditional response authored as part of a dialogue row. */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Dialogue Option"))
struct LOSTRUNIC_API FLRDialogueOption
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName OptionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FText Text;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName NextRowId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Conditions")
	FGameplayTagContainer RequiredTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Conditions")
	FGameplayTagContainer BlockedTags;
};

/** One stable dialogue line and its possible branches. */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Dialogue Row"))
struct LOSTRUNIC_API FLRDialogueRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName DialogueId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName SpeakerId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue", meta = (MultiLine = "true"))
	FText Text;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TSoftObjectPtr<UTexture2D> Portrait;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName NextRowId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TArray<FLRDialogueOption> Options;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Conditions")
	FGameplayTagContainer RequiredTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Conditions")
	FGameplayTagContainer BlockedTags;
};

/** A readable note addressed by a stable content ID. */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Reading Row"))
struct LOSTRUNIC_API FLRReadingRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reading")
	FName ReadingId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reading")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reading", meta = (MultiLine = "true"))
	FText Body;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reading")
	FGameplayTagContainer ChapterTags;
};

/** Stable map registration used by travel and save anchors. */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Map Registration"))
struct LOSTRUNIC_API FLRMapRegistration
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	FName MapId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	TSoftObjectPtr<UWorld> World;
};
