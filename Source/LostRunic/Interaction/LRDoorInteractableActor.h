/**
 * @file LRDoorInteractableActor.h
 * @brief One-shot door interaction that rotates a Blueprint-configured hinge pivot.
 */
#pragma once

#include "Interaction/LRWorldInteractionActor.h"

#include "LRDoorInteractableActor.generated.h"

class USceneComponent;

/** Opens a door by rotating DoorPivot; the Blueprint decides the mesh and hinge placement. */
UCLASS(Blueprintable, meta = (DisplayName = "Lost Runic Door Interactable"))
class LOSTRUNIC_API ALRDoorInteractableActor : public ALRWorldInteractionActor
{
	GENERATED_BODY()

public:
	/** Creates the Blueprint-visible hinge pivot. */
	ALRDoorInteractableActor();

	/** Caches the closed rotation after Blueprint defaults and construction are applied. */
	virtual void BeginPlay() override;

	/** Returns whether the door has already opened. */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Interaction")
	bool IsOpen() const { return bOpen; }

protected:
	/** Performs the door behavior after the base actor validates interaction state. */
	virtual FLRInteractionResult ExecuteInteractionInternal(AActor* interactor, FGameplayTag actionTag) override;

	/** Rotates once from the cached closed pivot rotation. */
	void OpenDoor();

	/** Blueprint hook for sounds, animation, or navigation updates after the state changes. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Lost Runic|Interaction")
	void OnDoorOpened();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Door", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> DoorPivot;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Door", meta = (ClampMin = "-360.0", ClampMax = "360.0", Units = "deg"))
	float OpenYawDegrees = 90.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Door", meta = (AllowPrivateAccess = "true"))
	bool bOpen = false;

private:
	FRotator ClosedPivotRotation = FRotator::ZeroRotator;
};
