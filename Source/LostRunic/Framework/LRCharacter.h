#pragma once

#include "GameFramework/Character.h"

#include "LRCharacter.generated.h"

class UCameraComponent;
class ULRInteractionComponent;
class ULRInventoryComponent;
class ULRLocomotionComponent;
class ULRStateComponent;
class ULRStatePresentationComponent;
class USpringArmComponent;

/** Thin player assembly root; independent gameplay capabilities live in components. */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Character"))
class LOSTRUNIC_API ALRCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ALRCharacter();

	void ApplyMoveInput(const FVector2D& input);

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Movement")
	ULRLocomotionComponent* GetLocomotionComponent() const { return Locomotion; }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|State")
	ULRStateComponent* GetStateComponent() const { return State; }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Interaction")
	ULRInteractionComponent* GetInteractionComponent() const { return Interaction; }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Inventory")
	ULRInventoryComponent* GetInventoryComponent() const { return Inventory; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULRLocomotionComponent> Locomotion;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULRStateComponent> State;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULRStatePresentationComponent> StatePresentation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULRInventoryComponent> Inventory;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULRInteractionComponent> Interaction;
};
