#pragma once

#include "Core/LRTypes.h"
#include "UObject/Object.h"

#include "LRHUDWidgetController.generated.h"

class ALRCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLRHUDPerceptionModeChanged, ELRPerceptionMode, mode, FGameplayTag, reason);

/** Mirrors presentation-safe character state into HUD layouts without exposing state mutation. */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic HUD Widget Controller"))
class LOSTRUNIC_API ULRHUDWidgetController : public UObject
{
	GENERATED_BODY()

public:
	void SetObservedCharacter(ALRCharacter* character);
	void Deinitialize();

	ELRPerceptionMode GetCurrentMode() const { return CurrentMode; }

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|UI")
	FLRHUDPerceptionModeChanged OnPerceptionModeChanged;

private:
	UFUNCTION()
	void HandleStateChanged(ELRPerceptionMode currentMode, FGameplayTag reason);

	TWeakObjectPtr<ALRCharacter> ObservedCharacter;
	ELRPerceptionMode CurrentMode = ELRPerceptionMode::Normal;
};
