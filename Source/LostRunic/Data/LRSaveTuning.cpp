#include "Data/LRSaveTuning.h"

#include "Core/LRValidation.h"

bool ULRSaveTuning::Validate(FString& outError) const
{
	if (RetryCount < 0 || RetryCount > 10)
	{
		outError = FString::Printf(TEXT("RetryCount must be in [0, 10]; actual %d."), RetryCount);
		return false;
	}

	if (ManualSlotCount < 1 || ManualSlotCount > 100)
	{
		outError = FString::Printf(TEXT("ManualSlotCount must be in [1, 100]; actual %d."), ManualSlotCount);
		return false;
	}

	return LRValidation::RequireRange(TEXT("AutoSaveDebounceSeconds"), AutoSaveDebounceSeconds, 0.0f, 60.0f, outError)
		&& LRValidation::RequireRange(TEXT("RetryDelaySeconds"), RetryDelaySeconds, 0.0f, 10.0f, outError);
}
