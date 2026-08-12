/**
 * @file LRInteractionPresentationComponent.cpp
 * @brief Implements state-to-visual mapping for world interaction feedback.
 */
#include "Interaction/LRInteractionPresentationComponent.h"

#include "Components/PrimitiveComponent.h"
#include "NiagaraComponent.h"

/** Creates an event-driven presentation component. */
ULRInteractionPresentationComponent::ULRInteractionPresentationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

/** Finds Blueprint mesh components explicitly marked for interaction outlining. */
void ULRInteractionPresentationComponent::BeginPlay()
{
	Super::BeginPlay();
	TInlineComponentArray<UPrimitiveComponent*> primitives(GetOwner());
	for (UPrimitiveComponent* primitive : primitives)
	{
		if (primitive && primitive->ComponentTags.Contains(TEXT("InteractionOutline")))
		{
			OutlineComponents.Add(primitive);
		}
	}
	ApplyVisualState();
}

/** Changes presentation state and avoids repeating render-state work. */
void ULRInteractionPresentationComponent::SetPresentationState(const ELRInteractionPresentationState newState)
{
	if (CurrentState != newState)
	{
		CurrentState = newState;
		ApplyVisualState();
	}
}

/** Associates the actor-owned Niagara component with this visual mapper. */
void ULRInteractionPresentationComponent::SetFarHintComponent(UNiagaraComponent* component)
{
	FarHintComponent = component;
	ApplyVisualState();
}

/** Maps state thresholds to particle activation and white-outline CustomDepth. */
void ULRInteractionPresentationComponent::ApplyVisualState()
{
	const bool bShowHint = CurrentState != ELRInteractionPresentationState::None;
	const bool bShowOutline = CurrentState == ELRInteractionPresentationState::NearOutline
		|| CurrentState == ELRInteractionPresentationState::Focused;
	if (FarHintComponent)
	{
		FarHintComponent->SetActive(bShowHint, true);
	}
	for (UPrimitiveComponent* primitive : OutlineComponents)
	{
		if (primitive)
		{
			primitive->SetRenderCustomDepth(bShowOutline);
		}
	}
}
