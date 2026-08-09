#pragma once

#include "Data/LRTuningAsset.h"

#include "LRMovementTuning.generated.h"

/** Player locomotion speed, footstep cadence, and environmental hearing ranges. */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Movement Tuning"))
class LOSTRUNIC_API ULRMovementTuning : public ULRTuningAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Speed", meta = (ClampMin = "1.0", ClampMax = "1000.0", Units = "cm/s"))
	float SneakSpeed = 80.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Speed", meta = (ClampMin = "1.0", ClampMax = "1000.0", Units = "cm/s"))
	float WalkSpeed = 150.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Speed", meta = (ClampMin = "1.0", ClampMax = "1000.0", Units = "cm/s"))
	float RunSpeed = 250.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Footsteps", meta = (ClampMin = "10.0", ClampMax = "500.0", Units = "cm"))
	float WalkStepDistance = 90.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Footsteps", meta = (ClampMin = "10.0", ClampMax = "500.0", Units = "cm"))
	float RunStepDistance = 120.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Footsteps", meta = (ClampMin = "0.02", ClampMax = "1.0", Units = "s"))
	float SampleIntervalSeconds = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Noise", meta = (ClampMin = "0.0", ClampMax = "5000.0", Units = "cm"))
	float IndoorWalkNoiseRadius = 400.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Noise", meta = (ClampMin = "0.0", ClampMax = "5000.0", Units = "cm"))
	float IndoorRunNoiseRadius = 1200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Noise", meta = (ClampMin = "0.0", ClampMax = "5000.0", Units = "cm"))
	float OutdoorSneakGuardNoiseRadius = 600.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Noise", meta = (ClampMin = "0.0", ClampMax = "5000.0", Units = "cm"))
	float OutdoorAlertGuardNoiseRadius = 250.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Noise", meta = (ClampMin = "0.0", ClampMax = "5000.0", Units = "cm"))
	float InteractionNoiseRadius = 500.0f;

	virtual bool Validate(FString& outError) const override;
};
