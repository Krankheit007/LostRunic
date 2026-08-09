#pragma once

#include "Components/ActorComponent.h"
#include "Items/LRItemUseTarget.h"

#include "LRTestItemUseTargetComponent.generated.h"

/** Deterministic item-use receiver used only by runtime automation tests. */
UCLASS()
class ULRTestItemUseTargetComponent : public UActorComponent, public ILRItemUseTarget
{
	GENERATED_BODY()

public:
	virtual FGameplayTagContainer GetItemUseTargetTags_Implementation() override { return TargetTags; }
	virtual FLRItemUseResult ApplyItemUse_Implementation(const FLRItemUseRequest& request,
		ULRItemDefinition* definition) override;

	FGameplayTagContainer TargetTags;
	bool bShouldSucceed = true;
	int32 ApplyCount = 0;
};
