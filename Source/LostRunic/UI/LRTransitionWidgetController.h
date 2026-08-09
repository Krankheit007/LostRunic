#pragma once

#include "UObject/Object.h"

#include "LRTransitionWidgetController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRTransitionVisibilityChanged, bool, bVisible);

/** Owns transition-overlay visibility while travel/save code owns the actual transaction. */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Transition Widget Controller"))
class LOSTRUNIC_API ULRTransitionWidgetController : public UObject
{
	GENERATED_BODY()

public:
	void SetTransitionVisible(bool bVisible);
	bool IsTransitionVisible() const { return bTransitionVisible; }

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|UI")
	FLRTransitionVisibilityChanged OnTransitionVisibilityChanged;

private:
	bool bTransitionVisible = false;
};
