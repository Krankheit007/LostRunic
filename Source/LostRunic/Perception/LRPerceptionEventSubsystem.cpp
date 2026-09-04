/**
 * @file LRPerceptionEventSubsystem.cpp
 * @brief Implements the Perception pulse bus and ownership sequencing.
 */
#include "Perception/LRPerceptionEventSubsystem.h"

#include "Core/LRGameplayTags.h"
#include "Interaction/LRInteractionPresentationComponent.h"
#include "Perception/LRPerceptionAccentComponent.h"
#include "Perception/LRPerceptionPresentationComponent.h"
#include "Perception/LRPerceptionSoundSourceComponent.h"
#include "Stealth/LRNoiseEmitterComponent.h"

void ULRPerceptionEventSubsystem::Deinitialize()
{
	for (const TWeakObjectPtr<ULRNoiseEmitterComponent>& emitterReference : NoiseEmitters)
	{
		ULRNoiseEmitterComponent* emitter = emitterReference.Get();
		if (IsValid(emitter))
		{
			emitter->OnNoiseEmitted.RemoveDynamic(this, &ULRPerceptionEventSubsystem::HandleNoiseEmitted);
		}
	}
	for (const TWeakObjectPtr<ULRPerceptionAccentComponent>& accentReference : NarrativeAccents)
	{
		ULRPerceptionAccentComponent* accent = accentReference.Get();
		if (IsValid(accent) && bPerceptionActive)
		{
			accent->DeactivateNarrativeAccent();
		}
	}
	bPerceptionActive = false;
	Presentations.Reset();
	SoundSources.Reset();
	NarrativeAccents.Reset();
	InteractionPresentations.Reset();
	NoiseEmitters.Reset();
	Super::Deinitialize();
}

void ULRPerceptionEventSubsystem::PublishPulse(const FLRPerceptionPulseRequest& request)
{
	if (!bPerceptionActive || request.WorldLocation.ContainsNaN() || request.Intensity <= 0.0f)
	{
		return;
	}
	OnPerceptionPulse.Broadcast(request);
}

void ULRPerceptionEventSubsystem::RegisterPresentation(ULRPerceptionPresentationComponent* presentation)
{
	if (!IsValid(presentation))
	{
		return;
	}
	Presentations.AddUnique(TWeakObjectPtr<ULRPerceptionPresentationComponent>(presentation));
}

void ULRPerceptionEventSubsystem::UnregisterPresentation(ULRPerceptionPresentationComponent* presentation)
{
	Presentations.RemoveAll([presentation](const TWeakObjectPtr<ULRPerceptionPresentationComponent>& value)
	{
		return value.Get() == presentation;
	});
}

void ULRPerceptionEventSubsystem::RegisterSoundSource(ULRPerceptionSoundSourceComponent* source)
{
	if (!IsValid(source))
	{
		return;
	}
	SoundSources.AddUnique(TWeakObjectPtr<ULRPerceptionSoundSourceComponent>(source));
	if (bPerceptionActive && source->IsLooping())
	{
		source->TriggerPulse();
	}
}

void ULRPerceptionEventSubsystem::UnregisterSoundSource(ULRPerceptionSoundSourceComponent* source)
{
	SoundSources.RemoveAll([source](const TWeakObjectPtr<ULRPerceptionSoundSourceComponent>& value)
	{
		return value.Get() == source;
	});
}

void ULRPerceptionEventSubsystem::EmitActiveLoopingSources()
{
	for (const TWeakObjectPtr<ULRPerceptionSoundSourceComponent>& sourceReference : SoundSources)
	{
		ULRPerceptionSoundSourceComponent* source = sourceReference.Get();
		if (IsValid(source) && source->IsLooping())
		{
			source->TriggerPulse();
		}
	}
}

void ULRPerceptionEventSubsystem::RegisterNarrativeAccent(ULRPerceptionAccentComponent* accent)
{
	if (!IsValid(accent))
	{
		return;
	}
	NarrativeAccents.AddUnique(TWeakObjectPtr<ULRPerceptionAccentComponent>(accent));
	if (bPerceptionActive)
	{
		accent->ActivateNarrativeAccent();
	}
}

void ULRPerceptionEventSubsystem::UnregisterNarrativeAccent(ULRPerceptionAccentComponent* accent)
{
	if (IsValid(accent) && bPerceptionActive)
	{
		accent->DeactivateNarrativeAccent();
	}
	NarrativeAccents.RemoveAll([accent](const TWeakObjectPtr<ULRPerceptionAccentComponent>& value)
	{
		return value.Get() == accent;
	});
}

void ULRPerceptionEventSubsystem::RegisterInteractionPresentation(ULRInteractionPresentationComponent* presentation)
{
	if (!IsValid(presentation))
	{
		return;
	}
	InteractionPresentations.AddUnique(TWeakObjectPtr<ULRInteractionPresentationComponent>(presentation));
	if (bPerceptionActive)
	{
		presentation->SetInteractionPresentationSuppressed(true);
	}
}

void ULRPerceptionEventSubsystem::UnregisterInteractionPresentation(ULRInteractionPresentationComponent* presentation)
{
	InteractionPresentations.RemoveAll([presentation](const TWeakObjectPtr<ULRInteractionPresentationComponent>& value)
	{
		return value.Get() == presentation;
	});
}

void ULRPerceptionEventSubsystem::SetPerceptionActive(const bool bActive)
{
	if (bPerceptionActive == bActive)
	{
		return;
	}

	if (bActive)
	{
		for (const TWeakObjectPtr<ULRInteractionPresentationComponent>& presentationReference : InteractionPresentations)
		{
			ULRInteractionPresentationComponent* presentation = presentationReference.Get();
			if (IsValid(presentation))
			{
				presentation->SetInteractionPresentationSuppressed(true);
			}
		}
		bPerceptionActive = true;
		for (const TWeakObjectPtr<ULRPerceptionAccentComponent>& accentReference : NarrativeAccents)
		{
			ULRPerceptionAccentComponent* accent = accentReference.Get();
			if (IsValid(accent))
			{
				accent->ActivateNarrativeAccent();
			}
		}
		EmitActiveLoopingSources();
		return;
	}

	for (const TWeakObjectPtr<ULRPerceptionAccentComponent>& accentReference : NarrativeAccents)
	{
		ULRPerceptionAccentComponent* accent = accentReference.Get();
		if (IsValid(accent))
		{
			accent->DeactivateNarrativeAccent();
		}
	}
	bPerceptionActive = false;
	for (const TWeakObjectPtr<ULRInteractionPresentationComponent>& presentationReference : InteractionPresentations)
	{
		ULRInteractionPresentationComponent* presentation = presentationReference.Get();
		if (IsValid(presentation))
		{
			presentation->SetInteractionPresentationSuppressed(false);
		}
	}
}

void ULRPerceptionEventSubsystem::RegisterNoiseEmitter(ULRNoiseEmitterComponent* emitter)
{
	if (!IsValid(emitter) || NoiseEmitters.ContainsByPredicate([emitter](const TWeakObjectPtr<ULRNoiseEmitterComponent>& value)
	{
		return value.Get() == emitter;
	}))
	{
		return;
	}
	NoiseEmitters.Add(TWeakObjectPtr<ULRNoiseEmitterComponent>(emitter));
	emitter->OnNoiseEmitted.AddDynamic(this, &ULRPerceptionEventSubsystem::HandleNoiseEmitted);
}

void ULRPerceptionEventSubsystem::UnregisterNoiseEmitter(ULRNoiseEmitterComponent* emitter)
{
	if (IsValid(emitter))
	{
		emitter->OnNoiseEmitted.RemoveDynamic(this, &ULRPerceptionEventSubsystem::HandleNoiseEmitted);
	}
	NoiseEmitters.RemoveAll([emitter](const TWeakObjectPtr<ULRNoiseEmitterComponent>& value)
	{
		return value.Get() == emitter;
	});
}

void ULRPerceptionEventSubsystem::HandleNoiseEmitted(const FVector location, const float radius,
	const FGameplayTag reason)
{
	(void)radius;
	if (!IsVisualNoiseReason(reason))
	{
		return;
	}
	FLRPerceptionPulseRequest request;
	request.WorldLocation = location;
	// Existing NoiseEmitter radius is the AI hearing radius. A visual source must
	// opt in through PublishPulse with its own radius; zero resolves to the
	// Presentation tuning default in the presentation component.
	request.VisualRadiusCm = 0.0f;
	request.Intensity = 1.0f;
	request.Reason = reason;
	request.bRefreshExistingSource = false;
	PublishPulse(request);
}

bool ULRPerceptionEventSubsystem::IsVisualNoiseReason(const FGameplayTag& reason)
{
	return reason == LRGameplayTags::NoiseInteraction
		|| reason == LRGameplayTags::NoiseFootstepWalk
		|| reason == LRGameplayTags::NoiseFootstepRun
		|| reason == LRGameplayTags::NoiseFootstepWalkFaint
		|| reason == LRGameplayTags::NoiseFootstepRunIndoor;
}

void ULRPerceptionEventSubsystem::PruneRegistrations()
{
	Presentations.RemoveAll([](const TWeakObjectPtr<ULRPerceptionPresentationComponent>& value) { return !IsValid(value.Get()); });
	SoundSources.RemoveAll([](const TWeakObjectPtr<ULRPerceptionSoundSourceComponent>& value) { return !IsValid(value.Get()); });
	NarrativeAccents.RemoveAll([](const TWeakObjectPtr<ULRPerceptionAccentComponent>& value) { return !IsValid(value.Get()); });
	InteractionPresentations.RemoveAll([](const TWeakObjectPtr<ULRInteractionPresentationComponent>& value) { return !IsValid(value.Get()); });
	NoiseEmitters.RemoveAll([](const TWeakObjectPtr<ULRNoiseEmitterComponent>& value) { return !IsValid(value.Get()); });
}
