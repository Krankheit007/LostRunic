/**
 * @file LRWorldInteractionActor.h
 * @brief Common world-actor contract and shared components for interactable objects.
 */
#pragma once

#include "GameFramework/Actor.h"
#include "Interaction/LRInteractable.h"

#include "LRWorldInteractionActor.generated.h"

class UNiagaraComponent;
class USceneComponent;
class USphereComponent;
class ULRInteractionPresentationComponent;

/** Base actor for world objects that can be discovered, presented, focused, and executed. */
UCLASS(Abstract, Blueprintable, meta = (DisplayName = "Lost Runic World Interaction Actor"))
class LOSTRUNIC_API ALRWorldInteractionActor : public AActor, public ILRInteractable
{
	GENERATED_BODY()

public:
	/** Creates the shared root, query collision, Niagara, and presentation components. */
	ALRWorldInteractionActor();

	/** Returns no options when disabled or after one-shot completion. */
	virtual TArray<FLRInteractionOption> GetInteractionOptions_Implementation(AActor* interactor) override;

	/** Uses the actor transform as the shared interaction point. */
	virtual FVector GetInteractionLocation_Implementation() override;

	/** Uses the interaction collision as the default UI prompt anchor. */
	virtual USceneComponent* GetInteractionPromptAnchorComponent_Implementation() override;

	/** Validates common state before dispatching to the specific interaction behavior. */
	virtual FLRInteractionResult ExecuteInteraction_Implementation(AActor* interactor, FGameplayTag actionTag) override;

	/** Returns completion state without exposing mutation to Blueprints. */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Interaction")
	bool IsInteractionCompleted() const { return bInteractionCompleted; }

	/** Returns the presentation mapper for Blueprint assembly inspection. */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Interaction")
	ULRInteractionPresentationComponent* GetPresentationComponent() const { return PresentationComponent; }

protected:
	/** Checks the shared enabled/completed state and requested action. */
	virtual bool CanInteract(AActor* interactor, FGameplayTag actionTag) const;

	/** Executes the concrete behavior after common validation has passed. */
	virtual FLRInteractionResult ExecuteInteractionInternal(AActor* interactor, FGameplayTag actionTag);

	/** Marks this actor complete, removes it from future options, and notifies presentation hooks. */
	void CompleteInteraction();

	/** Called once after a successful interaction for Blueprint-only audiovisual effects. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Lost Runic|Interaction")
	void OnInteractionCompleted();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> InteractionCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraComponent> FarHintEffect;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULRInteractionPresentationComponent> PresentationComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	TArray<FLRInteractionOption> InteractionOptions;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	bool bInteractionEnabled = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	bool bOneShot = true;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	bool bInteractionCompleted = false;
};
