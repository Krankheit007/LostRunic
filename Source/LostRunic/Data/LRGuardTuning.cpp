#include "Data/LRGuardTuning.h"

#include "Core/LRValidation.h"

bool ULRGuardTuning::Validate(FString& outError) const
{
	if (!LRValidation::RequireRange(TEXT("InvestigateSpeed"), InvestigateSpeed, 1.0f, 1000.0f, outError)
		|| !LRValidation::RequireRange(TEXT("ChaseSpeed"), ChaseSpeed, 1.0f, 1000.0f, outError))
	{
		return false;
	}

	if (InvestigateSpeed > ChaseSpeed)
	{
		outError = TEXT("InvestigateSpeed must not exceed ChaseSpeed.");
		return false;
	}

	return LRValidation::RequireRange(TEXT("SightRadius"), SightRadius, 50.0f, 5000.0f, outError)
		&& LRValidation::RequireRange(TEXT("LoseSightRadius"), LoseSightRadius, 50.0f, 5000.0f, outError)
		&& LRValidation::RequireRange(TEXT("SightConeDegrees"), SightConeDegrees, 1.0f, 180.0f, outError)
		&& LRValidation::RequireRange(TEXT("HearingRangeMultiplier"), HearingRangeMultiplier, 0.0f, 10.0f, outError)
		&& LRValidation::RequireRange(TEXT("MaxHearingRange"), MaxHearingRange, 50.0f, 10000.0f, outError)
		&& LRValidation::RequireRange(TEXT("HearingAlertAmount"), HearingAlertAmount, 1, 11, outError)
		&& LRValidation::RequireRange(TEXT("SightAlertLevel"), SightAlertLevel, 1, 11, outError)
		&& LRValidation::RequireRange(TEXT("AlertDecayAmount"), AlertDecayAmount, 1, 11, outError)
		&& LRValidation::RequireRange(TEXT("InitialObserveSeconds"), InitialObserveSeconds, 0.1f, 30.0f, outError)
		&& LRValidation::RequireRange(TEXT("AlertDecayIntervalSeconds"), AlertDecayIntervalSeconds, 0.05f, 10.0f, outError)
		&& LRValidation::RequireRange(TEXT("SearchDurationSeconds"), SearchDurationSeconds, 0.1f, 60.0f, outError)
		&& LRValidation::RequireRange(TEXT("CaptureRadius"), CaptureRadius, 10.0f, 500.0f, outError)
		&& LRValidation::RequireRange(TEXT("CaptureCheckIntervalSeconds"), CaptureCheckIntervalSeconds, 0.02f, 1.0f, outError)
		&& LRValidation::RequireRange(TEXT("MoveAcceptanceRadius"), MoveAcceptanceRadius, 1.0f, 500.0f, outError)
		&& LoseSightRadius >= SightRadius;
}
