/**
 * @file LRNoiseEmitterComponent.cpp
 * @brief 实现按不可变 Footstep Reason 表达发声时步态的脚步、房间传播和互动噪声。
 */
#include "Stealth/LRNoiseEmitterComponent.h"

#include "AI/LRGuardAIController.h"
#include "AI/LRGuardTypes.h"
#include "Core/LRGameplayTags.h"
#include "Data/LRGameTuningSet.h"
#include "Data/LRMovementTuning.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "GameFramework/Pawn.h"
#include "Gameplay/LRLocomotionComponent.h"
#include "Gameplay/LRMovementRules.h"
#include "Gameplay/LRRoomVolume.h"
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
	const ULRGameInstanceSubsystem* subsystem = gameInstance
		? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	Tuning = subsystem && subsystem->GetTuningSet() ? subsystem->GetTuningSet()->Movement : nullptr;
	if (!ensureMsgf(Locomotion && Interaction && Tuning,
		TEXT("%s requires locomotion, interaction, and Movement tuning."), *GetNameSafe(this)))
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

void ULRNoiseEmitterComponent::EmitNoise(const FVector location, const float radius,
	const FGameplayTag reason)
{
	EmitNoiseWithPace(location, radius, reason, ELRMovementPace::Walk, false);
}

void ULRNoiseEmitterComponent::EmitNoiseWithPace(const FVector location, const float radius,
	const FGameplayTag reason, const ELRMovementPace sourcePace, const bool bHasSourcePace)
{
	// UAISense_Hearing transports the immutable Reason tag only. Direct room
	// propagation constructs FLRGuardNoiseStimulus and carries this snapshot.
	(void)sourcePace;
	(void)bHasSourcePace;
	if (!GetWorld() || radius <= 0.0f || !reason.IsValid())
	{
		return;
	}

	UAISense_Hearing::ReportNoiseEvent(GetWorld(), location, 1.0f, GetOwner(), radius,
		reason.GetTagName());
	OnNoiseEmitted.Broadcast(location, radius, reason);
}

void ULRNoiseEmitterComponent::HandleFootstep(const FVector location, const float radius,
	const FGameplayTag reason)
{
	const ELRMovementPace sourcePace = Locomotion
		? Locomotion->GetPace() : ELRMovementPace::Walk;
	if (reason == LRGameplayTags::NoiseFootstepRunIndoor)
	{
		ApplyIndoorRunNoise(location, sourcePace);
		return;
	}
	EmitNoiseWithPace(location, radius, reason, sourcePace, true);
}
void ULRNoiseEmitterComponent::ApplyIndoorRunNoise(const FVector location, const ELRMovementPace sourcePace)
{
	if (!GetWorld() || !Tuning)
	{
		return;
	}

	TArray<ALRRoomVolume*> rooms;
	ALRRoomVolume::FindRoomsAtLocation(GetWorld(), location, rooms);
	if (rooms.Num() == 0)
	{
		EmitNoiseWithPace(location, Tuning->IndoorRunNoiseRadius,
			LRGameplayTags::NoiseFootstepRunIndoor, sourcePace, true);
		return;
	}

	TMap<AActor*, ELRGuardNoisePropagationMode> recipients;
	for (const ALRRoomVolume* room : rooms)
	{
		for (const TWeakObjectPtr<AActor>& guardWeak : room->GetOverlappingGuards())
		{
			if (AActor* guard = guardWeak.Get())
			{
				recipients.FindOrAdd(guard) = ELRGuardNoisePropagationMode::CurrentRoom;
			}
		}
		for (const TWeakObjectPtr<ALRRoomVolume>& adjacentWeak : room->GetAdjacentRooms())
		{
			const ALRRoomVolume* adjacent = adjacentWeak.Get();
			if (!adjacent)
			{
				continue;
			}
			for (const TWeakObjectPtr<AActor>& guardWeak : adjacent->GetOverlappingGuards())
			{
				AActor* guard = guardWeak.Get();
				if (guard && !recipients.Contains(guard))
				{
					recipients.Add(guard, ELRGuardNoisePropagationMode::AdjacentRoom);
				}
			}
		}
	}

	for (const TPair<AActor*, ELRGuardNoisePropagationMode>& recipient : recipients)
	{
		const APawn* guardPawn = Cast<APawn>(recipient.Key);
		ALRGuardAIController* controller = guardPawn
			? Cast<ALRGuardAIController>(guardPawn->GetController()) : nullptr;
		if (!controller)
		{
			continue;
		}

		FLRGuardNoiseStimulus stimulus;
		stimulus.Source = GetOwner();
		stimulus.Location = location;
		stimulus.Reason = LRGameplayTags::NoiseFootstepRunIndoor;
		stimulus.PropagationMode = recipient.Value;
		stimulus.SourcePace = sourcePace;
		stimulus.bHasSourcePace = true;
		stimulus.TimeSeconds = GetWorld()->GetTimeSeconds();
		controller->ReceiveNoiseStimulus(stimulus);
	}
	OnNoiseEmitted.Broadcast(location, Tuning->IndoorRunNoiseRadius,
		LRGameplayTags::NoiseFootstepRunIndoor);
}

void ULRNoiseEmitterComponent::HandleInteraction(const FLRInteractionResult result)
{
	if (result.bSuccess && Tuning)
	{
		EmitNoise(GetOwner()->GetActorLocation(), Tuning->InteractionNoiseRadius,
			LRGameplayTags::NoiseInteraction);
	}
}
