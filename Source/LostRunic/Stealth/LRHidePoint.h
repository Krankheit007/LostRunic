#pragma once

#include "GameFramework/Actor.h"
#include "Interaction/LRInteractable.h"

#include "LRHidePoint.generated.h"

class USceneComponent;

/** Authored fixed or movement-preserving hiding location. */
UCLASS(Blueprintable, meta = (DisplayName = "Lost Runic Hide Point"))
class LOSTRUNIC_API ALRHidePoint : public AActor, public ILRInteractable
{
	GENERATED_BODY()

public:
	ALRHidePoint();

	virtual TArray<FLRInteractionOption> GetInteractionOptions_Implementation(AActor* interactor) override;
	virtual FVector GetInteractionLocation_Implementation() override;
	virtual FLRInteractionResult ExecuteInteraction_Implementation(AActor* interactor, FGameplayTag actionTag) override;

	FVector GetHideLocation() const { return GetActorLocation(); }
	FVector GetExitLocation() const { return GetActorLocation() + GetActorForwardVector() * ExitOffset; }
	bool AllowsMovement() const { return bAllowMovementWhileHidden; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hide", meta = (AllowPrivateAccess = "true"))
	FLRInteractionOption InteractionOption;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hide", meta = (AllowPrivateAccess = "true"))
	bool bAllowMovementWhileHidden = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hide", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "500.0", Units = "cm"))
	float ExitOffset = 100.0f;
};
