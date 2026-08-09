#pragma once

#include "Core/LRTypes.h"
#include "GameFramework/PlayerController.h"

#include "LRPlayerController.generated.h"

class UInputMappingContext;
class ULRInputConfig;
struct FInputActionValue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLRInputModeChanged, ELRInputMode, previousMode, ELRInputMode, currentMode);

/** Routes semantic Enhanced Input actions and owns exclusive input contexts. */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Player Controller"))
class LOSTRUNIC_API ALRPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Input")
	void SetLRInputMode(ELRInputMode newMode);

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Input")
	ELRInputMode GetLRInputMode() const { return InputMode; }

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Input")
	FLRInputModeChanged OnInputModeChanged;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<ULRInputConfig> InputConfig;

private:
	void HandleMove(const FInputActionValue& value);
	void HandleSneakToggle();
	void HandleRunStarted();
	void HandleRunStopped();
	void HandleCloseEyesStarted();
	void HandleCloseEyesStopped();
	void HandleOpenEyesStarted();
	void HandleOpenEyesStopped();
	UInputMappingContext* ResolveContext(ELRInputMode mode) const;
	void UpdateStateInputBlocker(ELRInputMode previousMode, ELRInputMode newMode);

	ELRInputMode InputMode = ELRInputMode::Gameplay;
};
