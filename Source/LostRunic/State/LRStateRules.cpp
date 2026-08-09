#include "State/LRStateRules.h"

#include "Core/LRGameplayTags.h"
#include "Data/LRStateTuning.h"

bool FLRStateInputGate::Press(const ELRStateRequestType inputType)
{
	if (inputType != ELRStateRequestType::CloseEyes && inputType != ELRStateRequestType::OpenEyes)
	{
		return false;
	}
	if (IsHeld(inputType))
	{
		return false;
	}

	SetHeld(inputType, true);
	if (ActiveInput != ELRStateRequestType::None || bWaitForAllReleased)
	{
		bWaitForAllReleased = true;
		return false;
	}

	ActiveInput = inputType;
	bThresholdConsumed = false;
	return true;
}

bool FLRStateInputGate::Release(const ELRStateRequestType inputType)
{
	SetHeld(inputType, false);
	const bool bCanceled = ActiveInput == inputType && !bThresholdConsumed;
	if (ActiveInput == inputType)
	{
		ActiveInput = ELRStateRequestType::None;
		bThresholdConsumed = false;
	}
	if (!AreAnyHeld())
	{
		bWaitForAllReleased = false;
	}
	return bCanceled;
}

bool FLRStateInputGate::ConsumeThreshold(const ELRStateRequestType inputType)
{
	if (ActiveInput != inputType || !IsHeld(inputType) || bThresholdConsumed)
	{
		return false;
	}
	bThresholdConsumed = true;
	return true;
}

void FLRStateInputGate::Reset()
{
	ActiveInput = ELRStateRequestType::None;
	bCloseEyesHeld = false;
	bOpenEyesHeld = false;
	bThresholdConsumed = false;
	bWaitForAllReleased = false;
}

bool FLRStateInputGate::IsHeld(const ELRStateRequestType inputType) const
{
	return inputType == ELRStateRequestType::CloseEyes ? bCloseEyesHeld
		: inputType == ELRStateRequestType::OpenEyes && bOpenEyesHeld;
}

void FLRStateInputGate::SetHeld(const ELRStateRequestType inputType, const bool bHeld)
{
	if (inputType == ELRStateRequestType::CloseEyes)
	{
		bCloseEyesHeld = bHeld;
	}
	else if (inputType == ELRStateRequestType::OpenEyes)
	{
		bOpenEyesHeld = bHeld;
	}
}

bool LRStateRules::IsTransitionAllowed(const ELRPerceptionMode currentMode, const FLRStateChangeRequest& request)
{
	if (currentMode == request.TargetMode)
	{
		return false;
	}
	if (request.RequestType == ELRStateRequestType::Death)
	{
		return currentMode != ELRPerceptionMode::Memory && request.TargetMode == ELRPerceptionMode::Memory;
	}
	if (request.RequestType == ELRStateRequestType::Narrative)
	{
		return (currentMode == ELRPerceptionMode::Perception && request.TargetMode == ELRPerceptionMode::Memory)
			|| (currentMode == ELRPerceptionMode::Memory && request.TargetMode == ELRPerceptionMode::Normal);
	}
	if (request.RequestType == ELRStateRequestType::CloseEyes)
	{
		return (currentMode == ELRPerceptionMode::Normal && request.TargetMode == ELRPerceptionMode::Perception)
			|| (currentMode == ELRPerceptionMode::Courage && request.TargetMode == ELRPerceptionMode::Normal);
	}
	if (request.RequestType == ELRStateRequestType::OpenEyes)
	{
		return (currentMode == ELRPerceptionMode::Normal && request.TargetMode == ELRPerceptionMode::Courage)
			|| (currentMode == ELRPerceptionMode::Perception && request.TargetMode == ELRPerceptionMode::Normal);
	}
	return false;
}

bool LRStateRules::ResolveEyeTransition(const ELRPerceptionMode currentMode, const ELRStateRequestType inputType,
	const ULRStateTuning& tuning, ELRPerceptionMode& outTargetMode, float& outHoldSeconds)
{
	if (currentMode == ELRPerceptionMode::Normal && inputType == ELRStateRequestType::CloseEyes)
	{
		outTargetMode = ELRPerceptionMode::Perception;
		outHoldSeconds = tuning.EnterHoldSeconds;
		return true;
	}
	if (currentMode == ELRPerceptionMode::Normal && inputType == ELRStateRequestType::OpenEyes)
	{
		outTargetMode = ELRPerceptionMode::Courage;
		outHoldSeconds = tuning.EnterHoldSeconds;
		return true;
	}
	if ((currentMode == ELRPerceptionMode::Perception && inputType == ELRStateRequestType::OpenEyes)
		|| (currentMode == ELRPerceptionMode::Courage && inputType == ELRStateRequestType::CloseEyes))
	{
		outTargetMode = ELRPerceptionMode::Normal;
		outHoldSeconds = tuning.ExitHoldSeconds;
		return true;
	}
	return false;
}

FGameplayTag LRStateRules::GetSourceTag(const ELRStateRequestType requestType)
{
	if (requestType == ELRStateRequestType::CloseEyes)
	{
		return LRGameplayTags::StateSourceInputCloseEyes;
	}
	if (requestType == ELRStateRequestType::OpenEyes)
	{
		return LRGameplayTags::StateSourceInputOpenEyes;
	}
	if (requestType == ELRStateRequestType::Death)
	{
		return LRGameplayTags::StateSourceDeath;
	}
	if (requestType == ELRStateRequestType::Narrative)
	{
		return LRGameplayTags::StateSourceNarrative;
	}
	return FGameplayTag();
}
