/**
 * @file LRPerceptionEventSubsystem.h
 * @brief World-scoped visual pulse bus and fixed ownership coordinator for Perception presentation.
 */
#pragma once

#include "Perception/LRPerceptionTypes.h"
#include "Subsystems/WorldSubsystem.h"

#include "LRPerceptionEventSubsystem.generated.h"

class ULRInteractionPresentationComponent;
class ULRNoiseEmitterComponent;
class ULRPerceptionAccentComponent;
class ULRPerceptionSoundSourceComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnLostRunicPerceptionPulse, const FLRPerceptionPulseRequest&);

/** Coordinates per-world Perception pulses and Narrative Accent/Interaction ownership. */
UCLASS()
class LOSTRUNIC_API ULRPerceptionEventSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	/** Publishes one visual pulse; pulses are discarded while Perception is inactive. */
	void PublishPulse(const FLRPerceptionPulseRequest& request);

	/** Registers loop sources and emits them immediately when Perception becomes active. */
	void RegisterSoundSource(ULRPerceptionSoundSourceComponent* source);
	void UnregisterSoundSource(ULRPerceptionSoundSourceComponent* source);
	void EmitActiveLoopingSources();

	/** Registers Stencil 3 accent owners; no general-purpose stencil manager is introduced. */
	void RegisterNarrativeAccent(ULRPerceptionAccentComponent* accent);
	void UnregisterNarrativeAccent(ULRPerceptionAccentComponent* accent);

	/** Registers interaction presentation writers for the Perception suppression gate. */
	void RegisterInteractionPresentation(ULRInteractionPresentationComponent* presentation);
	void UnregisterInteractionPresentation(ULRInteractionPresentationComponent* presentation);

	/** Enables/disables the Perception ownership phase in the required order. */
	void SetPerceptionActive(bool bActive);
	bool IsPerceptionActive() const { return bPerceptionActive; }

	/** Existing gameplay noise is bridged once here, instead of by each presentation component. */
	void RegisterNoiseEmitter(ULRNoiseEmitterComponent* emitter);
	void UnregisterNoiseEmitter(ULRNoiseEmitterComponent* emitter);

	FOnLostRunicPerceptionPulse OnPerceptionPulse;

private:
	UFUNCTION()
	void HandleNoiseEmitted(FVector location, float radius, FGameplayTag reason);


	static bool IsVisualNoiseReason(const FGameplayTag& reason);

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ULRPerceptionSoundSourceComponent>> SoundSources;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ULRPerceptionAccentComponent>> NarrativeAccents;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ULRInteractionPresentationComponent>> InteractionPresentations;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ULRNoiseEmitterComponent>> NoiseEmitters;

	bool bPerceptionActive = false;
};
