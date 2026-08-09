#pragma once

#include "Data/LRTuningAsset.h"

#include "LRGuardTuning.generated.h"

/** Guard perception, navigation, alert, and capture tuning. */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Guard Tuning"))
class LOSTRUNIC_API ULRGuardTuning : public ULRTuningAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Sight", meta = (ClampMin = "50.0", ClampMax = "5000.0", Units = "cm"))
	float SightRadius = 500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Sight", meta = (ClampMin = "1.0", ClampMax = "180.0", Units = "deg", ToolTip = "Full sight cone; UE perception receives half this value."))
	float SightConeDegrees = 45.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Hearing", meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float HearingRangeMultiplier = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Movement", meta = (ClampMin = "1.0", ClampMax = "1000.0", Units = "cm/s"))
	float InvestigateSpeed = 170.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Movement", meta = (ClampMin = "1.0", ClampMax = "1000.0", Units = "cm/s"))
	float ChaseSpeed = 300.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Alert", meta = (ClampMin = "0.1", ClampMax = "30.0", Units = "s"))
	float InitialObserveSeconds = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Alert", meta = (ClampMin = "0.05", ClampMax = "10.0", Units = "s"))
	float AlertDecayIntervalSeconds = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Alert", meta = (ClampMin = "0.1", ClampMax = "60.0", Units = "s"))
	float SearchDurationSeconds = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Capture", meta = (ClampMin = "10.0", ClampMax = "500.0", Units = "cm"))
	float CaptureRadius = 75.0f;

	virtual bool Validate(FString& outError) const override;
};
