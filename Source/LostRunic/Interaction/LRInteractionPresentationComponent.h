/**
 * @file LRInteractionPresentationComponent.h
 * @brief Maps interaction presentation state to Niagara and mesh outline effects.
 */
#pragma once

#include "Components/ActorComponent.h"
#include "Interaction/LRInteractionTypes.h"

#include "LRInteractionPresentationComponent.generated.h"

class UNiagaraComponent;
class UPrimitiveComponent;

/** Receives a state from ULRInteractionComponent and owns only visual feedback. */
UCLASS(ClassGroup = "Lost Runic", BlueprintType, meta = (BlueprintSpawnableComponent, DisplayName = "Lost Runic Interaction Presentation"))
class LOSTRUNIC_API ULRInteractionPresentationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Creates a dormant presentation component with no per-frame tick. */
	ULRInteractionPresentationComponent();

	/** Resolves actor components tagged InteractionOutline after Blueprint construction completes. */
	virtual void BeginPlay() override;

	/** Applies the requested state only when it changes. */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Interaction")
	void SetPresentationState(ELRInteractionPresentationState newState);

	/** Returns the currently applied visual state. */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Interaction")
	ELRInteractionPresentationState GetPresentationState() const { return CurrentState; }

	/** Registers the shared far-hint Niagara component created by the world actor. */
	void SetFarHintComponent(UNiagaraComponent* component);

private:
	/** Refreshes Niagara activation and CustomDepth without evaluating gameplay rules. */
	void ApplyVisualState();

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> FarHintComponent;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPrimitiveComponent>> OutlineComponents;

	ELRInteractionPresentationState CurrentState = ELRInteractionPresentationState::None;
};
