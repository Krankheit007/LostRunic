/**
 * @file LRPerceptionSoundSourceComponent.cpp
 * @brief Implements timer-driven Perception ambient sources.
 */
#include "Perception/LRPerceptionSoundSourceComponent.h"

#include "Core/LRGameplayTags.h"
#include "Data/LRGameTuningSet.h"
#include "Data/LRPresentationTuning.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "Perception/LRPerceptionEventSubsystem.h"
#include "Stealth/LRNoiseEmitterComponent.h"
#include "TimerManager.h"

ULRPerceptionSoundSourceComponent::ULRPerceptionSoundSourceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULRPerceptionSoundSourceComponent::BeginPlay()
{
	Super::BeginPlay();
	if (Reason.IsValid() == false)
	{
		Reason = LRGameplayTags::NoiseInteraction;
	}
	if (UWorld* world = GetWorld())
	{
		if (ULRPerceptionEventSubsystem* subsystem = world->GetSubsystem<ULRPerceptionEventSubsystem>())
		{
			subsystem->RegisterSoundSource(this);
		}
	}
	if (bLooping && bAutoStart)
	{
		StartLooping();
	}
}

void ULRPerceptionSoundSourceComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	StopLooping();
	if (UWorld* world = GetWorld())
	{
		if (ULRPerceptionEventSubsystem* subsystem = world->GetSubsystem<ULRPerceptionEventSubsystem>())
		{
			subsystem->UnregisterSoundSource(this);
		}
	}
	Super::EndPlay(endPlayReason);
}

void ULRPerceptionSoundSourceComponent::TriggerPulse()
{
	UWorld* world = GetWorld();
	if (!world || !GetOwner())
	{
		return;
	}
	ULRPerceptionEventSubsystem* subsystem = world->GetSubsystem<ULRPerceptionEventSubsystem>();
	if (!subsystem)
	{
		return;
	}
	const FVector location = GetOwner()->GetActorLocation();
	const float visualRadius = ResolveVisualRadius();
	if (bAlsoEmitToAI && AIHearingRadiusCm > 0.0f)
	{
		if (ULRNoiseEmitterComponent* emitter = GetOwner()->FindComponentByClass<ULRNoiseEmitterComponent>())
		{
			emitter->ReportNoiseToAI(location, AIHearingRadiusCm, Reason);
		}
	}

	FLRPerceptionPulseRequest request;
	request.SourceObject = this;
	request.WorldLocation = location;
	request.VisualRadiusCm = visualRadius;
	request.Intensity = Intensity;
	request.Reason = Reason;
	request.bRefreshExistingSource = bLooping && bRefreshExistingSource;
	subsystem->PublishPulse(request);
}

void ULRPerceptionSoundSourceComponent::StartLooping()
{
	if (!bLooping || !GetWorld())
	{
		return;
	}
	const float interval = FMath::Max(ResolveLoopInterval(), 0.001f);
	GetWorld()->GetTimerManager().SetTimer(LoopTimer, this,
		&ULRPerceptionSoundSourceComponent::HandleLoopPulse, interval, true, interval);
}

void ULRPerceptionSoundSourceComponent::StopLooping()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(LoopTimer);
	}
}

void ULRPerceptionSoundSourceComponent::HandleLoopPulse()
{
	TriggerPulse();
}

float ULRPerceptionSoundSourceComponent::ResolveVisualRadius() const
{
	if (VisualRadiusOverrideCm > 0.0f)
	{
		return VisualRadiusOverrideCm;
	}
	const UGameInstance* gameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const ULRGameInstanceSubsystem* subsystem = gameInstance
		? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	if (subsystem && subsystem->GetTuningSet() && subsystem->GetTuningSet()->Presentation)
	{
		return subsystem->GetTuningSet()->Presentation->NoiseRevealRadius;
	}
	return GetDefault<ULRPresentationTuning>()->NoiseRevealRadius;
}

float ULRPerceptionSoundSourceComponent::ResolveLoopInterval() const
{
	if (LoopIntervalSeconds > 0.0f)
	{
		return LoopIntervalSeconds;
	}
	const UGameInstance* gameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const ULRGameInstanceSubsystem* subsystem = gameInstance
		? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	if (subsystem && subsystem->GetTuningSet() && subsystem->GetTuningSet()->Presentation)
	{
		return subsystem->GetTuningSet()->Presentation->DefaultLoopIntervalSeconds;
	}
	return GetDefault<ULRPresentationTuning>()->DefaultLoopIntervalSeconds;
}
