#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "LRGuardDefinition.generated.h"

class ULRGuardTuning;
class UStateTree;

/** Guard identity and authored behavior assets; numeric rules live in GuardTuning. */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Guard Definition"))
class LOSTRUNIC_API ULRGuardDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard")
	FName GuardId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard")
	TObjectPtr<ULRGuardTuning> Tuning;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard")
	TSoftObjectPtr<UStateTree> Behavior;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif
};
