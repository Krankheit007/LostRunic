#pragma once

#include "Data/LRTuningAsset.h"

#include "LRSaveTuning.generated.h"

/** Save queue debounce, retry, and slot policy. */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Save Tuning"))
class LOSTRUNIC_API ULRSaveTuning : public ULRTuningAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Save|Autosave", meta = (ClampMin = "0.0", ClampMax = "60.0", Units = "s"))
	float AutoSaveDebounceSeconds = 7.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Save|Retry", meta = (ClampMin = "0", ClampMax = "10"))
	int32 RetryCount = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Save|Retry", meta = (ClampMin = "0.0", ClampMax = "10.0", Units = "s"))
	float RetryDelaySeconds = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Save|Slots", meta = (ClampMin = "1", ClampMax = "100"))
	int32 ManualSlotCount = 10;

	virtual bool Validate(FString& outError) const override;
};
