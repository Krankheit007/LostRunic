/**
 * @file LRGuardTuning.cpp
 * @brief Guard 调优值的范围和跨字段校验。
 */
#include "Data/LRGuardTuning.h"

#include "Core/LRValidation.h"

bool FLRGuardTuningSettings::Validate(FString& outError) const
{
	if (!LRValidation::RequireRange(TEXT("AttractAlertAmount"), AttractAlertAmount, 1, 10, outError)
		|| !LRValidation::RequireRange(TEXT("SuspiciousObserveSeconds"), SuspiciousObserveSeconds, 0.1f, 30.0f, outError)
		|| !LRValidation::RequireRange(TEXT("InvestigateObserveSeconds"), InvestigateObserveSeconds, 0.1f, 30.0f, outError)
		|| !LRValidation::RequireRange(TEXT("SightToChaseGraceSeconds"), SightToChaseGraceSeconds, 0.0f, 10.0f, outError)
		|| !LRValidation::RequireRange(TEXT("SuspiciousStimulusCooldownSeconds"), SuspiciousStimulusCooldownSeconds, 0.0f, 10.0f, outError)
		|| !LRValidation::RequireRange(TEXT("InvestigateStimulusCooldownSeconds"), InvestigateStimulusCooldownSeconds, 0.0f, 10.0f, outError)
		|| !LRValidation::RequireRange(TEXT("AlertDecayAmount"), AlertDecayAmount, 1, 10, outError)
		|| !LRValidation::RequireRange(TEXT("AlertDecayIntervalSeconds"), AlertDecayIntervalSeconds, 0.05f, 10.0f, outError)
		|| !LRValidation::RequireRange(TEXT("RoomRunAlertLevel"), RoomRunAlertLevel, 1, 5, outError)
		|| !LRValidation::RequireRange(TEXT("AdjacentRoomRunAlertAmount"), AdjacentRoomRunAlertAmount, 1, 10, outError)
		|| !LRValidation::RequireRange(TEXT("SightTrackingIntervalSeconds"), SightTrackingIntervalSeconds, 0.01f, 1.0f, outError)
		|| !LRValidation::RequireRange(TEXT("FirstAttractRunCooldownMultiplier"), FirstAttractRunCooldownMultiplier, 0.0f, 4.0f, outError)
		|| !LRValidation::RequireRange(TEXT("FirstAttractWalkCooldownMultiplier"), FirstAttractWalkCooldownMultiplier, 0.0f, 4.0f, outError)
		|| !LRValidation::RequireRange(TEXT("FirstAttractSneakCooldownMultiplier"), FirstAttractSneakCooldownMultiplier, 0.0f, 4.0f, outError)
		|| !LRValidation::RequireRange(TEXT("PatrolSpeed"), PatrolSpeed, 1.0f, 1000.0f, outError)
		|| !LRValidation::RequireRange(TEXT("InvestigateSpeed"), InvestigateSpeed, 1.0f, 1000.0f, outError)
		|| !LRValidation::RequireRange(TEXT("ChaseSpeed"), ChaseSpeed, 1.0f, 1000.0f, outError)
		|| !LRValidation::RequireRange(TEXT("MoveAcceptanceRadius"), MoveAcceptanceRadius, 1.0f, 500.0f, outError)
		|| !LRValidation::RequireRange(TEXT("InvestigateMoveRetargetDistanceCm"), InvestigateMoveRetargetDistanceCm, 1.0f, 1000.0f, outError)
		|| !LRValidation::RequireRange(TEXT("CaptureRadius"), CaptureRadius, 10.0f, 500.0f, outError))
	{
		return false;
	}

	if (!(PatrolSpeed <= InvestigateSpeed && InvestigateSpeed <= ChaseSpeed))
	{
		outError = TEXT("PatrolSpeed, InvestigateSpeed and ChaseSpeed must be non-decreasing.");
		return false;
	}
	return true;
}
