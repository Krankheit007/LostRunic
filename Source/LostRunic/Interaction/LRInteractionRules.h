#pragma once

#include "Interaction/LRInteractionTypes.h"

class ULRInteractionTuning;

namespace LRInteractionRules
{
	LOSTRUNIC_API int32 SelectBestCandidate(const TArray<FLRInteractionCandidateScore>& candidates,
		const ULRInteractionTuning& tuning);
	LOSTRUNIC_API ELRInteractionRange GetRange(float distance, float executeDistance,
		const ULRInteractionTuning& tuning);
	LOSTRUNIC_API bool IsFacingAllowed(float forwardDot, const ULRInteractionTuning& tuning);
}
