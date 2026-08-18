/**
 * @file LRWorldInteractionActor.cpp
 * @brief Implements shared lifecycle and execution behavior for world interactables.
 */
#include "Interaction/LRWorldInteractionActor.h"

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Core/LRGameplayTags.h"
#include "Interaction/LRInteractionPresentationComponent.h"
#include "NiagaraComponent.h"

/** Creates the common Blueprint assembly points and configures the dedicated Interaction object channel. */
ALRWorldInteractionActor::ALRWorldInteractionActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	InteractionCollision = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionCollision"));
	InteractionCollision->SetupAttachment(SceneRoot);
	InteractionCollision->InitSphereRadius(32.0f);
	InteractionCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionCollision->SetCollisionObjectType(ECC_GameTraceChannel1);
	InteractionCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionCollision->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Overlap);
	FarHintEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FarHintEffect"));
	FarHintEffect->SetupAttachment(SceneRoot);
	FarHintEffect->SetAutoActivate(false);
	PresentationComponent = CreateDefaultSubobject<ULRInteractionPresentationComponent>(TEXT("Presentation"));
	PresentationComponent->SetFarHintComponent(FarHintEffect);
}

/** Supplies content options only while the actor remains a visible interaction object. */
TArray<FLRInteractionOption> ALRWorldInteractionActor::GetInteractionOptions_Implementation(AActor* interactor)
{
	return bInteractionEnabled && (!bOneShot || !bInteractionCompleted) ? InteractionOptions : TArray<FLRInteractionOption>();
}

/** Provides a single common point for distance, facing, and occlusion tests. */
FVector ALRWorldInteractionActor::GetInteractionLocation_Implementation()
{
	return GetActorLocation();
}

/** Returns the collision component for UI placement without changing gameplay query semantics. */
USceneComponent* ALRWorldInteractionActor::GetInteractionPromptAnchorComponent_Implementation()
{
	return InteractionCollision;
}

/** Keeps common rejection logic out of concrete door and pickup implementations. */
FLRInteractionResult ALRWorldInteractionActor::ExecuteInteraction_Implementation(AActor* interactor,
	const FGameplayTag actionTag)
{
	FLRInteractionResult result;
	result.ActionTag = actionTag;
	if (!CanInteract(interactor, actionTag))
	{
		result.FailureReason = bInteractionCompleted ? LRGameplayTags::InteractionRejectCompleted
			: LRGameplayTags::InteractionRejectNoTarget;
		return result;
	}
	return ExecuteInteractionInternal(interactor, actionTag);
}

/** Verifies that the requested action belongs to this actor and the actor is not completed. */
bool ALRWorldInteractionActor::CanInteract(AActor* interactor, const FGameplayTag actionTag) const
{
	if (!interactor || !bInteractionEnabled || (bOneShot && bInteractionCompleted))
	{
		return false;
	}
	return InteractionOptions.ContainsByPredicate([actionTag](const FLRInteractionOption& option)
	{
		return option.ActionTag == actionTag;
	});
}

/** Returns a structured rejection for base actors without a concrete behavior. */
FLRInteractionResult ALRWorldInteractionActor::ExecuteInteractionInternal(AActor* interactor,
	const FGameplayTag actionTag)
{
	FLRInteractionResult result;
	result.ActionTag = actionTag;
	result.FailureReason = LRGameplayTags::InteractionRejectNoTarget;
	return result;
}

/** Commits one-shot completion and immediately removes this actor from the next scan. */
void ALRWorldInteractionActor::CompleteInteraction()
{
	bInteractionCompleted = true;
	OnInteractionCompleted();
}
