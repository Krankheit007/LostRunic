#pragma once

#include "Items/LRItemUseTypes.h"
#include "UObject/Object.h"

#include "LRItemUseResolver.generated.h"

class ULRInventoryComponent;

/** Executes the single transactional item-use path for every UI entry point. */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Item Use Resolver"))
class LOSTRUNIC_API ULRItemUseResolver : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(ULRInventoryComponent* inventory);
	FLRItemUseResult ResolveAtTime(const FLRItemUseRequest& request, double currentTimeSeconds);

private:
	UObject* FindTargetObject(UObject* target) const;
	FLRItemUseResult Reject(const FLRItemUseRequest& request, FGameplayTag reason) const;

	UPROPERTY(Transient)
	TObjectPtr<ULRInventoryComponent> Inventory;

	double LastCourageUseSeconds = -DBL_MAX;
};
