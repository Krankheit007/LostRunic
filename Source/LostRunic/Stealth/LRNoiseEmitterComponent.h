#pragma once

#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"

#include "LRNoiseEmitterComponent.generated.h"

class ULRInteractionComponent;
class ULRLocomotionComponent;
class ULRMovementTuning;
struct FLRInteractionResult;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FLRNoiseEmitted, FVector, location, float, radius, FGameplayTag, reason);

/** Converts gameplay movement and interaction events into unified AI Hearing stimuli. */
UCLASS(ClassGroup = "Lost Runic", BlueprintType, meta = (BlueprintSpawnableComponent, DisplayName = "Lost Runic Noise Emitter"))
class LOSTRUNIC_API ULRNoiseEmitterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULRNoiseEmitterComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Noise")
	void EmitNoise(FVector location, float radius, FGameplayTag reason);

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Noise")
	FLRNoiseEmitted OnNoiseEmitted;

private:
	UFUNCTION()
	void HandleFootstep(FVector location, float radius, FGameplayTag reason);

	UFUNCTION()
	void HandleInteraction(FLRInteractionResult result);

	UPROPERTY(Transient)
	TObjectPtr<ULRLocomotionComponent> Locomotion;

	UPROPERTY(Transient)
	TObjectPtr<ULRInteractionComponent> Interaction;

	UPROPERTY(Transient)
	TObjectPtr<ULRMovementTuning> Tuning;
};
