/** @file LRMainMenuHUD.h @brief 主菜单 Host HUD；沿用 PlayerController，但不创建玩法 Pawn。 */
#pragma once

#include "UI/LRHUD.h"
#include "UI/LRMainMenuWidgetController.h"

#include "LRMainMenuHUD.generated.h"

class ULRMainMenuWidget;

UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Main Menu HUD"))
class LOSTRUNIC_API ALRMainMenuHUD : public ALRHUD
{
	GENERATED_BODY()

public:
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
	virtual void InitializeForController(ALRPlayerController* playerController) override;
	virtual ULRScreenWidget* GetFocusableScreen(ELRInputMode inputMode) const override;

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Main Menu")
	ULRMainMenuWidgetController* GetMainMenuWidgetController() const { return MainMenuWidgetController; }
	TSubclassOf<ULRMainMenuWidget> GetMainMenuScreenClass() const { return MainMenuScreenClass; }

protected:
	virtual void ReturnFromSaveSelection() override;
	virtual bool ShouldUseProjectDefaultHUDScreen() const override { return false; }

	/** Existing WBP_MainMenu class configured on the dedicated HUD Blueprint. */
	UPROPERTY(EditDefaultsOnly, Category = "Widgets")
	TSubclassOf<ULRMainMenuWidget> MainMenuScreenClass;

private:
	UFUNCTION()
	void HandleLoadRequested();

	UPROPERTY(Transient)
	TObjectPtr<ULRMainMenuWidgetController> MainMenuWidgetController;

	UPROPERTY(Transient)
	TObjectPtr<ULRMainMenuWidget> MainMenuWidget;
};
