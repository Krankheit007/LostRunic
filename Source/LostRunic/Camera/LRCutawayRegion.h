#pragma once

#include "GameFramework/Actor.h"

#include "LRCutawayRegion.generated.h"

class ACharacter;
class UBoxComponent;
class ULRCutawayTargetComponent;

/** Explicit room/area authoring actor that contributes one independent Group request. */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Cutaway Region"))
class LOSTRUNIC_API ALRCutawayRegion : public AActor
{
	GENERATED_BODY()

public:
	ALRCutawayRegion();
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* overlappedComponent, AActor* otherActor,
		UPrimitiveComponent* otherComponent, int32 otherBodyIndex, bool bFromSweep, const FHitResult& sweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* overlappedComponent, AActor* otherActor,
		UPrimitiveComponent* otherComponent, int32 otherBodyIndex);

	void SetTargetsRequested(bool bRequested);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> RegionBounds;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Cutaway", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<ULRCutawayTargetComponent>> Targets;

	TMap<TWeakObjectPtr<ACharacter>, int32> PlayerOverlapCounts;
};
