#pragma once

#include "Components/ActorComponent.h"
#include "Items/LRItemUseTarget.h"

#include "LRCourageResponseComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRCourageKnockbackApplied, FVector, direction);

/** Receives non-lethal Courage item use and applies a tuned root-motion knockback. */
UCLASS(ClassGroup = "Lost Runic", BlueprintType, meta = (BlueprintSpawnableComponent, DisplayName = "Lost Runic Courage Response"))
class LOSTRUNIC_API ULRCourageResponseComponent : public UActorComponent, public ILRItemUseTarget
{
	GENERATED_BODY()

public:
	ULRCourageResponseComponent();

	virtual FGameplayTagContainer GetItemUseTargetTags_Implementation() override;
	virtual FLRItemUseResult ApplyItemUse_Implementation(const FLRItemUseRequest& request,
		ULRItemDefinition* definition) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Courage")
	bool bImmune = false;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Courage")
	FLRCourageKnockbackApplied OnKnockbackApplied;
};
