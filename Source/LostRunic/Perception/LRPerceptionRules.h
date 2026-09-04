/**
 * @file LRPerceptionRules.h
 * @brief Pure Perception timing and slot-selection rules used by runtime code and automation tests.
 */
#pragma once

#include "Perception/LRPerceptionTypes.h"

namespace LRPerceptionRules
{
	/** Returns true when a request is allowed to refresh the existing spatial slot. */
	LOSTRUNIC_API bool CanRefreshSlot(const FLRPerceptionEchoSlot& slot,
		const FLRPerceptionPulseRequest& request, float now, float mergeDistanceCm);

	/** Returns residue opacity, with the final dry interval derived from ExpireTime. */
	LOSTRUNIC_API float ComputeResidueFade(float remainingSeconds, float dryFadeDurationSeconds);

	/** Chooses an inactive slot, then an expired slot, then the earliest-expiring active slot. */
	LOSTRUNIC_API int32 SelectSlot(const TArray<FLRPerceptionEchoSlot>& slots, float now);
}
