/**
 * @file LRPerceptionAccentComponent.cpp
 * @brief Implements the narrow Narrative Accent CustomDepth ownership contract.
 */
#include "Perception/LRPerceptionAccentComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Core/LRCustomStencil.h"
#include "Perception/LRPerceptionEventSubsystem.h"

ULRPerceptionAccentComponent::ULRPerceptionAccentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULRPerceptionAccentComponent::BeginPlay()
{
	Super::BeginPlay();
	if (UWorld* world = GetWorld())
	{
		if (ULRPerceptionEventSubsystem* subsystem = world->GetSubsystem<ULRPerceptionEventSubsystem>())
		{
			subsystem->RegisterNarrativeAccent(this);
		}
	}
}

void ULRPerceptionAccentComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	if (UWorld* world = GetWorld())
	{
		if (ULRPerceptionEventSubsystem* subsystem = world->GetSubsystem<ULRPerceptionEventSubsystem>())
		{
			subsystem->UnregisterNarrativeAccent(this);
		}
	}
	DeactivateNarrativeAccent();
	Super::EndPlay(endPlayReason);
}

void ULRPerceptionAccentComponent::ActivateNarrativeAccent()
{
	if (bAccentActive || !GetOwner())
	{
		return;
	}
	CachePrimitives();
	for (const FLRPerceptionAccentPrimitiveState& state : PrimitiveStates)
	{
		if (UPrimitiveComponent* primitive = state.Primitive.Get())
		{
			primitive->SetCustomDepthStencilValue(LRCustomStencil::PerceptionNarrativeAccent);
			primitive->SetRenderCustomDepth(true);
		}
	}
	bAccentActive = true;
}

void ULRPerceptionAccentComponent::DeactivateNarrativeAccent()
{
	if (!bAccentActive)
	{
		return;
	}
	for (const FLRPerceptionAccentPrimitiveState& state : PrimitiveStates)
	{
		if (UPrimitiveComponent* primitive = state.Primitive.Get())
		{
			if (primitive->CustomDepthStencilValue == LRCustomStencil::PerceptionNarrativeAccent)
			{
				primitive->SetCustomDepthStencilValue(state.StencilValue);
				primitive->SetRenderCustomDepth(state.bRenderCustomDepth);
			}
		}
	}
	bAccentActive = false;
}

void ULRPerceptionAccentComponent::CachePrimitives()
{
	PrimitiveStates.Reset();
	TInlineComponentArray<UPrimitiveComponent*> primitives(GetOwner());
	for (UPrimitiveComponent* primitive : primitives)
	{
		if (!primitive || (!bUseAllOwnerPrimitives
			&& !primitive->ComponentHasTag(TEXT("PerceptionAccent"))
			&& !primitive->ComponentHasTag(TEXT("NarrativeAccent"))))
		{
			continue;
		}
		FLRPerceptionAccentPrimitiveState& state = PrimitiveStates.AddDefaulted_GetRef();
		state.Primitive = primitive;
		state.bRenderCustomDepth = primitive->bRenderCustomDepth;
		state.StencilValue = primitive->CustomDepthStencilValue;
	}
}
