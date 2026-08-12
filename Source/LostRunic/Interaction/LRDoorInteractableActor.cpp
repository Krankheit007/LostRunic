/**
 * @file LRDoorInteractableActor.cpp
 * @brief Implements the immediate 90-degree hinge-pivot door opening for the Home slice.
 */
#include "Interaction/LRDoorInteractableActor.h"

#include "Components/SceneComponent.h"
#include "Core/LRGameplayTags.h"

/** Creates a separate hinge pivot so the mesh never rotates around the actor origin by accident. */
ALRDoorInteractableActor::ALRDoorInteractableActor()
{
	DoorPivot = CreateDefaultSubobject<USceneComponent>(TEXT("DoorPivot"));
	DoorPivot->SetupAttachment(SceneRoot);
	FLRInteractionOption option;
	option.ActionTag = LRGameplayTags::InteractionActionInteract;
	InteractionOptions = { option };
}

/** Captures the Blueprint-defined closed orientation after the component hierarchy is finalized. */
void ALRDoorInteractableActor::BeginPlay()
{
	Super::BeginPlay();
	ClosedPivotRotation = DoorPivot ? DoorPivot->GetRelativeRotation() : FRotator::ZeroRotator;
}

/** Opens the door once and exposes a structured success result to the interaction component. */
FLRInteractionResult ALRDoorInteractableActor::ExecuteInteractionInternal(AActor* interactor,
	const FGameplayTag actionTag)
{
	FLRInteractionResult result;
	result.ActionTag = actionTag;
	if (bOpen)
	{
		result.FailureReason = LRGameplayTags::InteractionRejectCompleted;
		return result;
	}
	OpenDoor();
	result.bSuccess = true;
	return result;
}

/** Applies the configured opening yaw relative to the cached closed hinge rotation. */
void ALRDoorInteractableActor::OpenDoor()
{
	if (!DoorPivot)
	{
		return;
	}
	bOpen = true;
	DoorPivot->SetRelativeRotation(ClosedPivotRotation + FRotator(0.0f, OpenYawDegrees, 0.0f));
	CompleteInteraction();
	OnDoorOpened();
}
