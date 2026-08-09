#pragma once

#include "Data/LRTuningAsset.h"

#include "LRStateTuning.generated.h"

/** Timing and non-lethal Courage parameters for perception state rules. */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic State Tuning"))
class LOSTRUNIC_API ULRStateTuning : public ULRTuningAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "State|Input", meta = (ClampMin = "0.05", ClampMax = "5.0", UIMin = "0.1", UIMax = "2.0", Units = "s"))
	float EnterHoldSeconds = 0.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "State|Input", meta = (ClampMin = "0.05", ClampMax = "5.0", UIMin = "0.1", UIMax = "1.0", Units = "s"))
	float ExitHoldSeconds = 0.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "State|Presentation", meta = (ClampMin = "0.1", ClampMax = "10.0", UIMin = "0.5", UIMax = "3.0", Units = "s"))
	float PresentationSafetyTimeoutSeconds = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "State|Courage", meta = (ClampMin = "0.0", ClampMax = "10.0", UIMin = "0.0", UIMax = "3.0", Units = "s"))
	float CourageAttackCooldownSeconds = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "State|Courage", meta = (ClampMin = "0.0", ClampMax = "5.0", UIMin = "0.0", UIMax = "2.0", Units = "s"))
	float CourageKnockbackDurationSeconds = 0.6f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "State|Courage", meta = (ClampMin = "0.0", ClampMax = "3000.0", UIMin = "100.0", UIMax = "1200.0", Units = "cm/s"))
	float CourageKnockbackSpeed = 600.0f;

	virtual bool Validate(FString& outError) const override;
};
