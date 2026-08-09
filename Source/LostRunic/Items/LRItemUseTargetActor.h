#pragma once

#include "GameFramework/Actor.h"
#include "Interaction/LRInteractable.h"
#include "Items/LRItemUseTarget.h"

#include "LRItemUseTargetActor.generated.h"

/** Configurable door or mechanism that routes both interaction entry points to item use. */
UCLASS(Blueprintable, meta = (DisplayName = "Lost Runic Item Use Target"))
class LOSTRUNIC_API ALRItemUseTargetActor : public AActor, public ILRInteractable, public ILRItemUseTarget
{
	GENERATED_BODY()

public:
	ALRItemUseTargetActor();

	virtual TArray<FLRInteractionOption> GetInteractionOptions_Implementation(AActor* interactor) override;
	virtual FVector GetInteractionLocation_Implementation() override;
	virtual FLRInteractionResult ExecuteInteraction_Implementation(AActor* interactor, FGameplayTag actionTag) override;
	virtual FGameplayTagContainer GetItemUseTargetTags_Implementation() override;
	virtual FLRItemUseResult ApplyItemUse_Implementation(const FLRItemUseRequest& request,
		ULRItemDefinition* definition) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	FLRInteractionOption InteractionOption;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Use")
	FGameplayTagContainer TargetTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Use")
	FName EventId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Use")
	bool bOneShot = true;

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Item Use")
	bool IsCompleted() const { return bCompleted; }

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Lost Runic|Item Use", meta = (DisplayName = "Item Use Applied"))
	void OnItemUseApplied(const FLRItemUseRequest& request, ULRItemDefinition* definition);

private:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Item Use", meta = (AllowPrivateAccess = "true"))
	bool bCompleted = false;
};
