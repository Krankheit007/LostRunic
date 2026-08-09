#pragma once

#include "GameFramework/Character.h"

#include "LRCharacter.generated.h"

class UCameraComponent;
class ULRLocomotionComponent;
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

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULRLocomotionComponent> Locomotion;
};
