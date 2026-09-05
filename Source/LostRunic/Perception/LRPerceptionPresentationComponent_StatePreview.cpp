/**
 * @file LRPerceptionPresentationComponent_StatePreview.cpp
 * @brief Handles the presentation-only Normal preview used while opening the eyes.
 */
#include "Perception/LRPerceptionPresentationComponent.h"

#include "State/LRStateComponent.h"

void ULRPerceptionPresentationComponent::HandleStateHoldStarted(
	const ELRStateRequestType inputType, const ELRPerceptionMode targetMode, const float holdSeconds)
{
	(void)holdSeconds;
	if (inputType != ELRStateRequestType::OpenEyes
		|| targetMode != ELRPerceptionMode::Normal
		|| !StateComponent
		|| StateComponent->GetCurrentMode() != ELRPerceptionMode::Perception
		|| bNormalEyeHoldPreviewActive)
	{
		return;
	}

	bNormalEyeHoldPreviewActive = true;
	PreviewNormalForEyeHold();
}

void ULRPerceptionPresentationComponent::HandleStateHoldCanceled(const ELRStateRequestType inputType)
{
	if (inputType == ELRStateRequestType::OpenEyes && bNormalEyeHoldPreviewActive)
	{
		RestorePerceptionAfterEyeHold();
	}
}

void ULRPerceptionPresentationComponent::HandleStateChangeRejected(
	const FLRStateChangeRequest request, const FGameplayTag reason)
{
	(void)reason;
	if (request.RequestType == ELRStateRequestType::OpenEyes
		&& request.TargetMode == ELRPerceptionMode::Normal
		&& bNormalEyeHoldPreviewActive)
	{
		RestorePerceptionAfterEyeHold();
	}
}

void ULRPerceptionPresentationComponent::PreviewNormalForEyeHold()
{
	// Keep Perception ownership, Echo slots and the event bus intact. The accepted state event
	// performs the real exit cleanup; this only changes what is visible behind the opening lids.
	bBlendActive = false;
	SetComponentTickEnabled(false);
	CurrentBlend = 0.0f;
	WriteStyleParameters();
}

void ULRPerceptionPresentationComponent::RestorePerceptionAfterEyeHold()
{
	bNormalEyeHoldPreviewActive = false;
	if (StateComponent && StateComponent->GetCurrentMode() == ELRPerceptionMode::Perception)
	{
		AddBlendable();
		CurrentBlend = 1.0f;
		WriteStyleParameters();
	}
}
