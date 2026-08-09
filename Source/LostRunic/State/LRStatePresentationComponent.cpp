#include "State/LRStatePresentationComponent.h"

#include "State/LRStateComponent.h"

ULRStatePresentationComponent::ULRStatePresentationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULRStatePresentationComponent::BeginPlay()
{
	Super::BeginPlay();
	StateComponent = GetOwner() ? GetOwner()->FindComponentByClass<ULRStateComponent>() : nullptr;
	if (ensureMsgf(StateComponent, TEXT("%s requires a sibling LRStateComponent."), *GetNameSafe(this)))
	{
		StateComponent->OnStateChanging.AddDynamic(this, &ULRStatePresentationComponent::HandleStateChanging);
	}
}

void ULRStatePresentationComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	if (StateComponent)
	{
		StateComponent->OnStateChanging.RemoveDynamic(this, &ULRStatePresentationComponent::HandleStateChanging);
	}
	Super::EndPlay(endPlayReason);
}

void ULRStatePresentationComponent::CompleteStatePresentation()
{
	if (StateComponent)
	{
		StateComponent->NotifyPresentationComplete();
	}
}

void ULRStatePresentationComponent::HandleStateChanging(const ELRPerceptionMode previousMode,
	const ELRPerceptionMode nextMode, const FGameplayTag reason)
{
	PresentStateChange(previousMode, nextMode, reason);
}
