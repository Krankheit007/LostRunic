#pragma once

#include "Core/LRTypes.h"
#include "GameFramework/PlayerController.h"
#include "Items/LRItemUseTypes.h"
#include "UI/LRUITypes.h"

#include "LRPlayerController.generated.h"

class UInputMappingContext;
class ULRInputConfig;
class ULRPlayerUIComponent;
struct FInputActionValue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLRInputModeChanged, ELRInputMode, previousMode, ELRInputMode, currentMode);

/** Routes semantic Enhanced Input actions and owns exclusive input contexts. */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Player Controller"))
class LOSTRUNIC_API ALRPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ALRPlayerController();

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* pawn) override;

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Input")
	void SetLRInputMode(ELRInputMode newMode);

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Input")
	ELRInputMode GetLRInputMode() const { return InputMode; }

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|UI")
	void OpenMenuScreen(ELRScreenType screen);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|UI")
	void CloseMenuScreen();

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Inventory")
	FLRItemUseResult UseInventoryItemFromMenu(FName itemId);

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
	void HandleInteract();
	void HandleQuickSlot1();
	void HandleQuickSlot2();
	void HandleQuickSlot3();
	void HandleQuickSlot4();
	void HandleUseSelectedQuickSlot();
	void HandlePreviousQuickSlot();
	void HandleNextQuickSlot();
	void HandleConfirm();
	void HandleCancel();
	void HandleOpenJournal();
	void HandlePause();
	void UseQuickSlot(int32 slotIndex);
	UInputMappingContext* ResolveContext(ELRInputMode mode) const;
	void UpdateStateInputBlocker(ELRInputMode previousMode, ELRInputMode newMode);
	void ConfigureViewportInput(ELRInputMode newMode);
	class ALRHUD* GetLRHUD() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULRPlayerUIComponent> PlayerUI;

	ELRInputMode InputMode = ELRInputMode::Gameplay;
};
