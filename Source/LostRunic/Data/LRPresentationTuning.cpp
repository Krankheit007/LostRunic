#include "Data/LRPresentationTuning.h"

#include "Core/LRValidation.h"

bool ULRPresentationTuning::Validate(FString& outError) const
{
	return LRValidation::RequireRange(TEXT("PerceptionRevealRadius"), PerceptionRevealRadius, 0.0f, 5000.0f, outError)
		&& LRValidation::RequireRange(TEXT("NoiseRevealRadius"), NoiseRevealRadius, 0.0f, 5000.0f, outError)
		&& LRValidation::RequireRange(TEXT("NoiseRevealDurationSeconds"), NoiseRevealDurationSeconds, 0.0f, 30.0f, outError)
		&& LRValidation::RequireRange(TEXT("PerceptionBlendWeight"), PerceptionBlendWeight, 0.0f, 1.0f, outError)
		&& LRValidation::RequireRange(TEXT("CourageBlendWeight"), CourageBlendWeight, 0.0f, 1.0f, outError);
}
