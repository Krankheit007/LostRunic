#include "Data/LRMovementTuning.h"

#include "Core/LRValidation.h"

bool ULRMovementTuning::Validate(FString& outError) const
{
	if (!LRValidation::RequireRange(TEXT("SneakSpeed"), SneakSpeed, 1.0f, 1000.0f, outError)
		|| !LRValidation::RequireRange(TEXT("WalkSpeed"), WalkSpeed, 1.0f, 1000.0f, outError)
		|| !LRValidation::RequireRange(TEXT("RunSpeed"), RunSpeed, 1.0f, 1000.0f, outError))
	{
		return false;
	}

	if (!(SneakSpeed <= WalkSpeed && WalkSpeed <= RunSpeed))
	{
		outError = TEXT("Movement speeds must satisfy SneakSpeed <= WalkSpeed <= RunSpeed.");
		return false;
	}

	return LRValidation::RequireRange(TEXT("WalkStepDistance"), WalkStepDistance, 10.0f, 500.0f, outError)
		&& LRValidation::RequireRange(TEXT("RunStepDistance"), RunStepDistance, 10.0f, 500.0f, outError)
		&& LRValidation::RequireRange(TEXT("SampleIntervalSeconds"), SampleIntervalSeconds, 0.02f, 1.0f, outError)
		&& LRValidation::RequireRange(TEXT("IndoorWalkNoiseRadius"), IndoorWalkNoiseRadius, 0.0f, 5000.0f, outError)
		&& LRValidation::RequireRange(TEXT("IndoorRunNoiseRadius"), IndoorRunNoiseRadius, 0.0f, 5000.0f, outError)
		&& LRValidation::RequireRange(TEXT("OutdoorSneakGuardNoiseRadius"), OutdoorSneakGuardNoiseRadius, 0.0f, 5000.0f, outError)
		&& LRValidation::RequireRange(TEXT("OutdoorAlertGuardNoiseRadius"), OutdoorAlertGuardNoiseRadius, 0.0f, 5000.0f, outError);
}
