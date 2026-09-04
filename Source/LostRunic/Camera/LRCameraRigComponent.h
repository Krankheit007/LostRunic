#pragma once

#include "Components/ActorComponent.h"

#include "LRCameraRigComponent.generated.h"

class USpringArmComponent;

/** Owns the fixed-distance top-down camera contract and rare authored distance transitions. */
UCLASS(ClassGroup = "Lost Runic", BlueprintType, meta = (BlueprintSpawnableComponent, DisplayName = "Lost Runic Camera Rig"))
class LOSTRUNIC_API ULRCameraRigComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULRCameraRigComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* tickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Camera")
	bool SetSpecialCameraDistance(float distanceCm, float blendSeconds = -1.0f);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Camera")
	void RestoreDefaultCameraDistance(float blendSeconds = -1.0f);

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Camera")
	float GetCurrentCameraDistance() const;

private:
	void StartTransition(float targetDistance, float blendSeconds);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Distance", meta = (AllowPrivateAccess = "true", ClampMin = "300.0", ClampMax = "1400.0", Units = "cm"))
	float DefaultCameraDistance = 700.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Distance", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "5.0", Units = "s"))
	float DefaultBlendSeconds = 0.25f;

	UPROPERTY(Transient)
	TObjectPtr<USpringArmComponent> CameraBoom;

	float TransitionStartDistance = 700.0f;
	float TransitionTargetDistance = 700.0f;
	float TransitionElapsed = 0.0f;
	float TransitionDuration = 0.0f;
};
