#pragma once

#include "Core/LRTypes.h"
#include "GameFramework/Actor.h"

#include "LRNoiseArea.generated.h"

class UBoxComponent;

/** Volume-like actor that changes the player's authored footstep environment. */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Noise Area"))
class LOSTRUNIC_API ALRNoiseArea : public AActor
{
	GENERATED_BODY()

public:
	ALRNoiseArea();

private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* component, AActor* otherActor, UPrimitiveComponent* otherComponent,
		int32 otherBodyIndex, bool bFromSweep, const FHitResult& sweepResult);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Noise", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> Bounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Noise", meta = (AllowPrivateAccess = "true"))
	ELRNoiseEnvironment Environment = ELRNoiseEnvironment::Indoor;
};
