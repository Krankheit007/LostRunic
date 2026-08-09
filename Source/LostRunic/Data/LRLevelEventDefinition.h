#pragma once

#include "Core/LRTypes.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "LRLevelEventDefinition.generated.h"

/** Stable narrative event definition with explicit persistence behavior. */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Level Event Definition"))
class LOSTRUNIC_API ULRLevelEventDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event")
	FName EventId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event|Conditions")
	FGameplayTagContainer RequiredTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event|Conditions")
	FGameplayTagContainer BlockedTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event|Rules")
	bool bOneShot = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event|Save")
	ELRSavePolicy SavePolicy = ELRSavePolicy::None;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif
};
