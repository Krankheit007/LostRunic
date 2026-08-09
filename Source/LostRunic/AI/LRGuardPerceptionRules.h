#pragma once

class ULRGuardTuning;

namespace LRGuardPerceptionRules
{
	LOSTRUNIC_API bool CanConfirmSight(float distance, float forwardDot, bool bOccluded, bool bHidden,
		const ULRGuardTuning& tuning);
	LOSTRUNIC_API bool CanHear(float distance, float sourceRadius, const ULRGuardTuning& tuning);
}
