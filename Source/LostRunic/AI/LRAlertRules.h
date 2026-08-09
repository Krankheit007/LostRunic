#pragma once

#include "AI/LRGuardTypes.h"

namespace LRAlertRules
{
	LOSTRUNIC_API int32 ApplyDelta(int32 currentLevel, int32 delta);
	LOSTRUNIC_API ELRGuardBehaviorState ResolveState(int32 alertLevel, bool bHasSight, bool bSearching);
	LOSTRUNIC_API bool ShouldDecay(float secondsSinceStimulus, float observeSeconds, bool bHasSight);
}
