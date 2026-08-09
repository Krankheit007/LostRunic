#pragma once

#include "Components/ActorComponent.h"
#include "Core/LRTypes.h"
#include "GameplayTagContainer.h"

#include "LRStatePresentationComponent.generated.h"

class ULRStateComponent;

/** Bridges state events to authored presentation and returns completion callbacks. */
UCLASS(ClassGroup = "Lost Runic", BlueprintType, meta = (BlueprintSpawnableComponent, DisplayName = "Lost Runic State Presentation"))
class LOSTRUNIC_API ULRStatePresentationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULRStatePresentationComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|State|Presentation")
	void CompleteStatePresentation();

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Lost Runic|State|Presentation", meta = (DisplayName = "Present State Change"))
	void PresentStateChange(ELRPerceptionMode previousMode, ELRPerceptionMode nextMode, FGameplayTag reason);

private:
	UFUNCTION()
	void HandleStateChanging(ELRPerceptionMode previousMode, ELRPerceptionMode nextMode, FGameplayTag reason);

	UPROPERTY(Transient)
	TObjectPtr<ULRStateComponent> StateComponent;
};
