#pragma once

#include "Data/LRTuningAsset.h"

#include "LRPresentationTuning.generated.h"

/** State presentation radii, durations, and post-process weights. */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Presentation Tuning"))
class LOSTRUNIC_API ULRPresentationTuning : public ULRTuningAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Perception", meta = (ClampMin = "0.0", ClampMax = "5000.0", Units = "cm"))
	float PerceptionRevealRadius = 450.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Perception", meta = (ClampMin = "0.0", ClampMax = "5000.0", Units = "cm"))
	float NoiseRevealRadius = 200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Perception", meta = (ClampMin = "0.0", ClampMax = "30.0", Units = "s"))
	float NoiseRevealDurationSeconds = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|PostProcess", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PerceptionBlendWeight = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|PostProcess", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CourageBlendWeight = 1.0f;

	virtual bool Validate(FString& outError) const override;
};
