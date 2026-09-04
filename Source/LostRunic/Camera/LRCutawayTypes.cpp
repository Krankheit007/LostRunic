#include "Camera/LRCutawayTypes.h"

void FLRCutawayChannelState::Retarget(const float newTarget, const double now, const float hideDuration,
	const float restoreDuration, const bool bImmediate)
{
	Evaluate(now);
	const float clampedTarget = FMath::Clamp(newTarget, 0.0f, 1.0f);
	if (bImmediate)
	{
		CurrentAmount = clampedTarget;
		TargetAmount = clampedTarget;
		TransitionStartAmount = clampedTarget;
		TransitionStartTime = now;
		TransitionDuration = 0.0f;
		return;
	}

	TransitionStartAmount = CurrentAmount;
	TargetAmount = clampedTarget;
	TransitionStartTime = now;
	const float baseDuration = TargetAmount > CurrentAmount ? hideDuration : restoreDuration;
	TransitionDuration = FMath::Max(0.0f, baseDuration) * FMath::Abs(TargetAmount - CurrentAmount);
	if (TransitionDuration <= UE_KINDA_SMALL_NUMBER)
	{
		CurrentAmount = TargetAmount;
	}
}
bool FLRCutawayChannelState::Evaluate(const double now)
{
	const float previous = CurrentAmount;
	if (TransitionDuration <= UE_KINDA_SMALL_NUMBER)
	{
		CurrentAmount = TargetAmount;
	}
	else
	{
		const float alpha = FMath::Clamp(static_cast<float>((now - TransitionStartTime) / TransitionDuration), 0.0f, 1.0f);
		CurrentAmount = FMath::Lerp(TransitionStartAmount, TargetAmount, alpha);
		if (alpha >= 1.0f)
		{
			TransitionDuration = 0.0f;
		}
	}
	return !FMath::IsNearlyEqual(previous, CurrentAmount);
}
bool FLRCutawayChannelState::IsTransitioning() const
{
	return TransitionDuration > UE_KINDA_SMALL_NUMBER && !FMath::IsNearlyEqual(CurrentAmount, TargetAmount);
}
