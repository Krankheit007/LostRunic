#pragma once

#include "Data/LRTuningAsset.h"

#include "LRUITuning.generated.h"

/** UI timing values that affect feedback and dialogue pacing. */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic UI Tuning"))
class LOSTRUNIC_API ULRUITuning : public ULRTuningAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Dialogue", meta = (ClampMin = "1.0", ClampMax = "240.0", UIMin = "5.0", UIMax = "120.0", ToolTip = "Number of visible dialogue characters revealed per second."))
	float TypewriterCharactersPerSecond = 30.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Feedback", meta = (ClampMin = "0.1", ClampMax = "10.0", Units = "s"))
	float FailureMessageSeconds = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Input", meta = (ClampMin = "0.05", ClampMax = "2.0", Units = "s"))
	float NavigationRepeatSeconds = 0.2f;

	virtual bool Validate(FString& outError) const override;
};
