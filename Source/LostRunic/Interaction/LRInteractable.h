#pragma once

#include "Interaction/LRInteractionTypes.h"
#include "UObject/Interface.h"

#include "LRInteractable.generated.h"

UINTERFACE(BlueprintType, meta = (DisplayName = "Lost Runic Interactable"))
class LOSTRUNIC_API ULRInteractable : public UInterface
{
	GENERATED_BODY()
};

/** Capability contract for targets discovered by ULRInteractionComponent. */
class LOSTRUNIC_API ILRInteractable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Lost Runic|Interaction")
	TArray<FLRInteractionOption> GetInteractionOptions(AActor* interactor);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Lost Runic|Interaction")
	FVector GetInteractionLocation();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Lost Runic|Interaction")
	FLRInteractionResult ExecuteInteraction(AActor* interactor, FGameplayTag actionTag);
};
