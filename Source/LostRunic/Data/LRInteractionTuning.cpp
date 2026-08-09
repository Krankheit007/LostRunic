#include "Data/LRInteractionTuning.h"

#include "Core/LRValidation.h"

bool ULRInteractionTuning::Validate(FString& outError) const
{
	if (!(ExecuteDistance <= OutlineDistance && OutlineDistance <= FarHintDistance))
	{
		outError = TEXT("Interaction distances must satisfy ExecuteDistance <= OutlineDistance <= FarHintDistance.");
		return false;
	}

	return LRValidation::RequireRange(TEXT("ExecuteDistance"), ExecuteDistance, 1.0f, 5000.0f, outError)
		&& LRValidation::RequireRange(TEXT("OutlineDistance"), OutlineDistance, 1.0f, 5000.0f, outError)
		&& LRValidation::RequireRange(TEXT("FarHintDistance"), FarHintDistance, 1.0f, 5000.0f, outError)
		&& LRValidation::RequireRange(TEXT("FacingConeDegrees"), FacingConeDegrees, 1.0f, 360.0f, outError)
		&& LRValidation::RequireRange(TEXT("QueryIntervalSeconds"), QueryIntervalSeconds, 0.02f, 1.0f, outError);
}
