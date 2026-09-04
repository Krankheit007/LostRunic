/**
 * @file LRPerceptionSoundSourceComponent.h
 * @brief Timer-driven one-shot/looping ambient producer for Perception pulses.
 */
#pragma once

#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"

#include "LRPerceptionSoundSourceComponent.generated.h"

class UNiagaraSystem;

/** Creates spatial Perception events without owning or polling presentation state. */
UCLASS(ClassGroup = "Lost Runic", BlueprintType, meta = (BlueprintSpawnableComponent, DisplayName = "Lost Runic Perception Sound Source"))
class LOSTRUNIC_API ULRPerceptionSoundSourceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULRPerceptionSoundSourceComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

	/** Emits one event immediately when Perception is active. */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Perception|Sound")
	void TriggerPulse();

	/** Starts a game-time timer for this looping source. */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Perception|Sound")
	void StartLooping();

	/** Stops the looping timer without destroying the component. */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Perception|Sound")
	void StopLooping();

	bool IsLooping() const { return bLooping; }
	TSoftObjectPtr<UNiagaraSystem> GetNiagaraOverride() const { return NiagaraOverride; }

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Perception|Sound")
	bool bLooping = false;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Perception|Sound")
	bool bAutoStart = true;

	/** A positive value overrides PresentationTuning.DefaultLoopIntervalSeconds; zero uses that shared default. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Perception|Sound", meta = (ClampMin = "0.0", ClampMax = "60.0", Units = "s"))
	float LoopIntervalSeconds = 0.0f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Perception|Sound", meta = (ClampMin = "0.0", ClampMax = "5000.0", Units = "cm"))
	float VisualRadiusOverrideCm = 0.0f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Perception|Sound", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Intensity = 1.0f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Perception|Sound")
	FGameplayTag Reason;

	/** Looping sources only: refresh an existing slot when the source remains nearby. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Perception|Sound")
	bool bRefreshExistingSource = true;

	/** Route an ambient pulse through a sibling NoiseEmitter for AI hearing as well. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Perception|Sound")
	bool bAlsoEmitToAI = false;

	/** Explicit AI hearing radius used only when bAlsoEmitToAI is enabled; zero emits no AI event. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Perception|Sound", meta = (ClampMin = "0.0", ClampMax = "5000.0", Units = "cm"))
	float AIHearingRadiusCm = 0.0f;

	/** Optional decorative Niagara override; it never controls reveal rules. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Perception|Sound|VFX")
	TSoftObjectPtr<UNiagaraSystem> NiagaraOverride;

private:
	UFUNCTION()
	void HandleLoopPulse();

	float ResolveVisualRadius() const;
	float ResolveLoopInterval() const;

	FTimerHandle LoopTimer;
};
