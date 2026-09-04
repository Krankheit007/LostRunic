/**
 * @file LRInteractionPresentationComponent.cpp
 * @brief Implements state-to-visual mapping for world interaction feedback.
 */
#include "Interaction/LRInteractionPresentationComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Core/LRCustomStencil.h"
#include "Engine/World.h"
#include "NiagaraComponent.h"
#include "Perception/LRPerceptionEventSubsystem.h"

/** Creates an event-driven presentation component. */
ULRInteractionPresentationComponent::ULRInteractionPresentationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

/** Finds Blueprint mesh components explicitly marked for interaction outlining. */
void ULRInteractionPresentationComponent::BeginPlay()
{
	Super::BeginPlay();
	RefreshOutlineComponents();
	if (UWorld* world = GetWorld())
	{
		if (ULRPerceptionEventSubsystem* subsystem = world->GetSubsystem<ULRPerceptionEventSubsystem>())
		{
			subsystem->RegisterInteractionPresentation(this);
		}
	}
	ApplyVisualState();
}

void ULRInteractionPresentationComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	if (UWorld* world = GetWorld())
	{
		if (ULRPerceptionEventSubsystem* subsystem = world->GetSubsystem<ULRPerceptionEventSubsystem>())
		{
			subsystem->UnregisterInteractionPresentation(this);
		}
	}
	Super::EndPlay(endPlayReason);
}

/** Rebuilds the interaction outline cache and assigns the project-owned stencil value. */
void ULRInteractionPresentationComponent::RefreshOutlineComponents()
{
	OutlineComponents.Reset();
	TInlineComponentArray<UPrimitiveComponent*> primitives(GetOwner());
	for (UPrimitiveComponent* primitive : primitives)
	{
		if (primitive && primitive->ComponentTags.Contains(TEXT("InteractionOutline")))
		{
			if (!bInteractionPresentationSuppressed
				&& primitive->CustomDepthStencilValue != LRCustomStencil::PerceptionNarrativeAccent)
			{
				primitive->SetCustomDepthStencilValue(LRCustomStencil::InteractionSelected);
			}
			OutlineComponents.Add(primitive);
		}
	}
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

void ULRInteractionPresentationComponent::SetInteractionPresentationSuppressed(const bool bSuppressed)
{
	if (bInteractionPresentationSuppressed == bSuppressed)
	{
		return;
	}
	bInteractionPresentationSuppressed = bSuppressed;
	if (bInteractionPresentationSuppressed)
	{
		if (FarHintComponent)
		{
			FarHintComponent->SetActive(false, true);
		}
		for (UPrimitiveComponent* primitive : OutlineComponents)
		{
			if (primitive && primitive->CustomDepthStencilValue == LRCustomStencil::InteractionSelected)
			{
				primitive->SetRenderCustomDepth(false);
			}
		}
		return;
	}
	ApplyVisualState();
}

/** Associates the actor-owned Niagara component with this visual mapper. */
void ULRInteractionPresentationComponent::SetFarHintComponent(UNiagaraComponent* component)
{
	FarHintComponent = component;
	ApplyVisualState();
}

/** Resolves an optional component picker reference without making presentation mandatory. */
USceneComponent* ULRInteractionPresentationComponent::ResolvePromptAnchorComponent(USceneComponent* defaultAnchor) const
{
	const bool bHasConfiguredOverride = !PromptAnchorOverride.ComponentProperty.IsNone()
		|| !PromptAnchorOverride.PathToComponent.IsEmpty()
		|| PromptAnchorOverride.OtherActor.IsValid()
		|| PromptAnchorOverride.OverrideComponent.IsValid();
	if (bHasConfiguredOverride && GetOwner())
	{
		if (USceneComponent* overrideAnchor = Cast<USceneComponent>(PromptAnchorOverride.GetComponent(GetOwner())))
		{
			return overrideAnchor;
		}
	}
	return defaultAnchor;
}

/** Resolves the instance override against the shared interaction tuning value. */
float ULRInteractionPresentationComponent::ResolvePromptZOffset(const float defaultOffset) const
{
	return bOverridePromptZOffset ? PromptZOffsetOverride : defaultOffset;
}

/** Maps state thresholds to particle activation and white-outline CustomDepth. */
void ULRInteractionPresentationComponent::ApplyVisualState()
{
	if (bInteractionPresentationSuppressed)
	{
		if (FarHintComponent)
		{
			FarHintComponent->SetActive(false, true);
		}
		return;
	}
	const bool bShowHint = CurrentState != ELRInteractionPresentationState::None;
	const bool bShowOutline = CurrentState == ELRInteractionPresentationState::NearOutline
		|| CurrentState == ELRInteractionPresentationState::Focused;
	if (FarHintComponent)
	{
		FarHintComponent->SetActive(bShowHint, true);
	}
	for (UPrimitiveComponent* primitive : OutlineComponents)
	{
		if (primitive && primitive->CustomDepthStencilValue == LRCustomStencil::InteractionSelected)
		{
			primitive->SetRenderCustomDepth(bShowOutline);
		}
	}
}
