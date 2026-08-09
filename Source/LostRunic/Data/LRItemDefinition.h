#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "LRItemDefinition.generated.h"

class UTexture2D;

/** Immutable item content referenced by a stable ItemId. */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Item Definition"))
class LOSTRUNIC_API ULRItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FName ItemId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Rules")
	FGameplayTagContainer ItemTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Rules")
	FGameplayTagContainer AllowedActionTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Rules")
	FGameplayTagContainer AllowedTargetTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Rules")
	bool bConsumable = false;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif
};
