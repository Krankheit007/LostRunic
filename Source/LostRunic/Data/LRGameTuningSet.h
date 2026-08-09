#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "LRGameTuningSet.generated.h"

class ULRGuardTuning;
class ULRInteractionTuning;
class ULRMovementTuning;
class ULRPresentationTuning;
class ULRSaveTuning;
class ULRStateTuning;
class ULRUITuning;

/** Required tuning assets and the single runtime source for gameplay parameters. */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Game Tuning Set"))
class LOSTRUNIC_API ULRGameTuningSet : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tuning")
	TObjectPtr<ULRStateTuning> State;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tuning")
	TObjectPtr<ULRMovementTuning> Movement;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tuning")
	TObjectPtr<ULRInteractionTuning> Interaction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tuning")
	TObjectPtr<ULRGuardTuning> Guard;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tuning")
	TObjectPtr<ULRSaveTuning> Save;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tuning")
	TObjectPtr<ULRUITuning> UI;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tuning")
	TObjectPtr<ULRPresentationTuning> Presentation;

	bool Validate(FString& outError) const;
	void LogSources() const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif
};
