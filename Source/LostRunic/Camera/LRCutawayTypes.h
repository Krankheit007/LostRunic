#pragma once

#include "CoreMinimal.h"

#include "LRCutawayTypes.generated.h"

UENUM(BlueprintType)
enum class ELRCutawayRequestType : uint8
{
	Local,
	Group,
	Foreground
};

namespace LR::Cutaway
{
	/** Converts a world-space centimeter distance to a primitive-local height fraction. */
	LOSTRUNIC_API float ConvertWorldHeightToLocalFraction(
		float worldHeightCm, float localBoundsHeightCm, float componentScaleZ);
}

/** Reversible scalar transition shared by all cutaway request channels. */
USTRUCT()
struct LOSTRUNIC_API FLRCutawayChannelState
{
	GENERATED_BODY()

	float CurrentAmount = 0.0f;
	float TargetAmount = 0.0f;
	float TransitionStartAmount = 0.0f;
	double TransitionStartTime = 0.0;
	float TransitionDuration = 0.0f;

	void Retarget(float newTarget, double now, float hideDuration, float restoreDuration, bool bImmediate = false);
	bool Evaluate(double now);
	bool IsTransitioning() const;
};
