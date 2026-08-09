#include "AI/LRAlertRules.h"

namespace
{
	constexpr int32 MinAlertLevel = 0;
	constexpr int32 MaxAlertLevel = 11;
	constexpr int32 SuspiciousMaxLevel = 5;
}

int32 LRAlertRules::ApplyDelta(const int32 currentLevel, const int32 delta)
{
	return FMath::Clamp(currentLevel + delta, MinAlertLevel, MaxAlertLevel);
}

ELRGuardBehaviorState LRAlertRules::ResolveState(const int32 alertLevel, const bool bHasSight, const bool bSearching)
{
	if (alertLevel <= MinAlertLevel)
	{
		return ELRGuardBehaviorState::IdlePatrol;
	}
	if (bHasSight && alertLevel >= MaxAlertLevel)
	{
		return ELRGuardBehaviorState::Chase;
	}
	if (bSearching || alertLevel >= MaxAlertLevel)
	{
		return ELRGuardBehaviorState::Search;
	}
	return alertLevel <= SuspiciousMaxLevel
		? ELRGuardBehaviorState::Suspicious : ELRGuardBehaviorState::Investigate;
}

bool LRAlertRules::ShouldDecay(const float secondsSinceStimulus, const float observeSeconds, const bool bHasSight)
{
	return !bHasSight && secondsSinceStimulus >= observeSeconds;
}
