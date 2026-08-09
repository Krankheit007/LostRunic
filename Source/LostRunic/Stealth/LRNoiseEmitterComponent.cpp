#include "Stealth/LRNoiseEmitterComponent.h"

#include "Core/LRGameplayTags.h"
#include "Data/LRGameTuningSet.h"
#include "Data/LRMovementTuning.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "Gameplay/LRLocomotionComponent.h"
#include "Interaction/LRInteractionComponent.h"
#include "Interaction/LRInteractionTypes.h"
#include "Perception/AISense_Hearing.h"

ULRNoiseEmitterComponent::ULRNoiseEmitterComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULRNoiseEmitterComponent::BeginPlay()
{
	Super::BeginPlay();
	Locomotion = GetOwner() ? GetOwner()->FindComponentByClass<ULRLocomotionComponent>() : nullptr;
	Interaction = GetOwner() ? GetOwner()->FindComponentByClass<ULRInteractionComponent>() : nullptr;
	const UGameInstance* gameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const ULRGameInstanceSubsystem* subsystem = gameInstance ? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	Tuning = subsystem && subsystem->GetTuningSet() ? subsystem->GetTuningSet()->Movement : nullptr;
	if (!ensureMsgf(Locomotion && Interaction && Tuning, TEXT("%s requires locomotion, interaction, and Movement tuning."),
		*GetNameSafe(this)))
	{
		return;
	}
	Locomotion->OnFootstep.AddDynamic(this, &ULRNoiseEmitterComponent::HandleFootstep);
	Interaction->OnInteractionExecuted.AddDynamic(this, &ULRNoiseEmitterComponent::HandleInteraction);
}

void ULRNoiseEmitterComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	if (Locomotion)
	{
		Locomotion->OnFootstep.RemoveDynamic(this, &ULRNoiseEmitterComponent::HandleFootstep);
	}
	if (Interaction)
	{
		Interaction->OnInteractionExecuted.RemoveDynamic(this, &ULRNoiseEmitterComponent::HandleInteraction);
	}
	Super::EndPlay(endPlayReason);
}

void ULRNoiseEmitterComponent::EmitNoise(const FVector location, const float radius, const FGameplayTag reason)
{
	if (!GetWorld() || radius <= 0.0f || !reason.IsValid())
	{
		return;
	}
	UAISense_Hearing::ReportNoiseEvent(GetWorld(), location, 1.0f, GetOwner(), radius, reason.GetTagName());
	OnNoiseEmitted.Broadcast(location, radius, reason);
}

void ULRNoiseEmitterComponent::HandleFootstep(const FVector location, const float radius, const FGameplayTag reason)
{
	EmitNoise(location, radius, reason);
}

void ULRNoiseEmitterComponent::HandleInteraction(const FLRInteractionResult result)
{
	if (result.bSuccess && Tuning)
	{
		EmitNoise(GetOwner()->GetActorLocation(), Tuning->InteractionNoiseRadius, LRGameplayTags::NoiseInteraction);
	}
}
