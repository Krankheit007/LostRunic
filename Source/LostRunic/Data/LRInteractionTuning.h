#pragma once

#include "Data/LRTuningAsset.h"

#include "LRInteractionTuning.generated.h"

/** Candidate scan distances, cadence, and facing rules. */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Interaction Tuning"))
class LOSTRUNIC_API ULRInteractionTuning : public ULRTuningAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction|Distance", meta = (ClampMin = "1.0", ClampMax = "5000.0", Units = "cm"))
	float FarHintDistance = 1000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction|Distance", meta = (ClampMin = "1.0", ClampMax = "5000.0", Units = "cm"))
	float OutlineDistance = 500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction|Distance", meta = (ClampMin = "1.0", ClampMax = "5000.0", Units = "cm"))
	float ExecuteDistance = 200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction|Selection", meta = (ClampMin = "1.0", ClampMax = "360.0", Units = "deg"))
	float FacingConeDegrees = 90.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction|Selection", meta = (ClampMin = "0.02", ClampMax = "1.0", Units = "s"))
	float QueryIntervalSeconds = 0.1f;

	virtual bool Validate(FString& outError) const override;
};
