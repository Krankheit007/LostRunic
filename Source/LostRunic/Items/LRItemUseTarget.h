#pragma once

#include "Items/LRItemUseTypes.h"
#include "UObject/Interface.h"

#include "LRItemUseTarget.generated.h"

class ULRItemDefinition;

UINTERFACE(BlueprintType, meta = (DisplayName = "Lost Runic Item Use Target"))
class LOSTRUNIC_API ULRItemUseTarget : public UInterface
{
	GENERATED_BODY()
};

/** Capability contract for any actor or component that receives an item transaction. */
class LOSTRUNIC_API ILRItemUseTarget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Lost Runic|Item Use")
	FGameplayTagContainer GetItemUseTargetTags();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Lost Runic|Item Use")
	FLRItemUseResult ApplyItemUse(const FLRItemUseRequest& request, ULRItemDefinition* definition);
};
