#pragma once

#include "State/LRStateTypes.h"

class ULRStateTuning;

/** Tracks physical eye buttons independently from state and presentation. */
class LOSTRUNIC_API FLRStateInputGate
{
public:
	bool Press(ELRStateRequestType inputType);
	bool Release(ELRStateRequestType inputType);
	bool ConsumeThreshold(ELRStateRequestType inputType);
	void Reset();

	ELRStateRequestType GetActiveInput() const { return ActiveInput; }
	bool IsWaitingForAllReleased() const { return bWaitForAllReleased; }

private:
	bool IsHeld(ELRStateRequestType inputType) const;
	void SetHeld(ELRStateRequestType inputType, bool bHeld);
	bool AreAnyHeld() const { return bCloseEyesHeld || bOpenEyesHeld; }

	ELRStateRequestType ActiveInput = ELRStateRequestType::None;
	bool bCloseEyesHeld = false;
	bool bOpenEyesHeld = false;
	bool bThresholdConsumed = false;
	bool bWaitForAllReleased = false;
};

namespace LRStateRules
{
	LOSTRUNIC_API bool IsTransitionAllowed(ELRPerceptionMode currentMode, const FLRStateChangeRequest& request);
	LOSTRUNIC_API bool ResolveEyeTransition(ELRPerceptionMode currentMode, ELRStateRequestType inputType,
		const ULRStateTuning& tuning, ELRPerceptionMode& outTargetMode, float& outHoldSeconds);
	LOSTRUNIC_API FGameplayTag GetSourceTag(ELRStateRequestType requestType);
}
