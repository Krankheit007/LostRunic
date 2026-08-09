#include "AI/LRGuardPerceptionRules.h"

#include "Data/LRGuardTuning.h"

bool LRGuardPerceptionRules::CanConfirmSight(const float distance, const float forwardDot, const bool bOccluded,
	const bool bHidden, const ULRGuardTuning& tuning)
{
	const float halfAngleRadians = FMath::DegreesToRadians(tuning.SightConeDegrees * 0.5f);
	return distance <= tuning.SightRadius && forwardDot >= FMath::Cos(halfAngleRadians) && !bOccluded && !bHidden;
}

bool LRGuardPerceptionRules::CanHear(const float distance, const float sourceRadius, const ULRGuardTuning& tuning)
{
	return distance <= sourceRadius * tuning.HearingRangeMultiplier;
}
