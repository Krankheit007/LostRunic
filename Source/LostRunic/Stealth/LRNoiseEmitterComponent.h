/**
 * @file LRNoiseEmitterComponent.h
 * @brief 发布带发声时刻步态快照的统一噪声事件。
 */
#pragma once

#include "Components/ActorComponent.h"
#include "Core/LRTypes.h"
#include "GameplayTagContainer.h"

#include "LRNoiseEmitterComponent.generated.h"

class ULRInteractionComponent;
class ULRLocomotionComponent;
class ULRMovementTuning;
struct FLRInteractionResult;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FLRNoiseEmitted, FVector, location, float, radius, FGameplayTag, reason);

/** 玩家脚步和互动噪声的统一发射器。 */
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

	/** Reports an AI hearing event without broadcasting the Perception visual bridge. */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Noise")
	void ReportNoiseToAI(FVector location, float radius, FGameplayTag reason);

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Noise")
	FLRNoiseEmitted OnNoiseEmitted;

private:
	UFUNCTION()
	void HandleFootstep(FVector location, float radius, FGameplayTag reason);

	void EmitNoiseWithPace(FVector location, float radius, FGameplayTag reason,
		ELRMovementPace sourcePace, bool bHasSourcePace);
	void ApplyIndoorRunNoise(FVector location, ELRMovementPace sourcePace);

	UFUNCTION()
	void HandleInteraction(FLRInteractionResult result);

	UPROPERTY(Transient)
	TObjectPtr<ULRLocomotionComponent> Locomotion;

	UPROPERTY(Transient)
	TObjectPtr<ULRInteractionComponent> Interaction;

	UPROPERTY(Transient)
	TObjectPtr<ULRMovementTuning> Tuning;
};
