/**
 * @file LRGuardConstants.h
 * @brief Shared Guard domain bounds used by Core, Data and AI without reversing module-layer dependencies.
 */
#pragma once

#include "CoreMinimal.h"

namespace LRGuardConstants
{
	/** Inclusive lower bound for the Guard alert scale. */
	inline constexpr int32 MinAlertLevel = 0;
	/** Inclusive upper bound for the Guard alert scale. */
	inline constexpr int32 MaxAlertLevel = 11;
	/** Noise and attraction can raise alert only through the red investigation band. */
	inline constexpr int32 MaxNoiseAlertLevel = 10;
}
