#pragma once

#include "Components/ActorComponent.h"
#include "Core/LRTypes.h"
#include "GameplayTagContainer.h"

#include "LRLocomotionComponent.generated.h"

class ACharacter;
class ULRMovementTuning;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FLRFootstepEvent, FVector, location, float, radius, FGameplayTag, noiseTag);

/** Owns movement pace and distance-based footsteps without requiring Actor Tick. */
UCLASS(ClassGroup = "Lost Runic", BlueprintType, meta = (BlueprintSpawnableComponent, DisplayName = "Lost Runic Locomotion"))
class LOSTRUNIC_API ULRLocomotionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULRLocomotionComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Movement")
	void SetPace(ELRMovementPace newPace);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Movement")
	void ToggleSneak();

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Movement")
	void StartRun();

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Movement")
	void StopRun();

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Movement")
	ELRMovementPace GetPace() const { return Pace; }

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Movement")
	void SetNoiseEnvironment(ELRNoiseEnvironment newEnvironment) { NoiseEnvironment = newEnvironment; }

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Noise")
	FLRFootstepEvent OnFootstep;

private:
	void SampleTravelDistance();
	float GetStepDistance() const;
	float GetNoiseRadius() const;

	UPROPERTY(Transient)
	TObjectPtr<ACharacter> Character;

	UPROPERTY(Transient)
	TObjectPtr<ULRMovementTuning> Tuning;

	ELRMovementPace Pace = ELRMovementPace::Walk;
	ELRMovementPace PaceBeforeRun = ELRMovementPace::Walk;
	ELRNoiseEnvironment NoiseEnvironment = ELRNoiseEnvironment::Indoor;
	FVector LastSampleLocation = FVector::ZeroVector;
	float DistanceSinceFootstep = 0.0f;
	FTimerHandle SampleTimer;
};
