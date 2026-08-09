#pragma once

#include "GameFramework/HUD.h"
#include "UI/LRUITypes.h"

#include "LRHUD.generated.h"

class ALRCharacter;
class ALRPlayerController;
class ULRDialogueWidgetController;
class ULRHUDWidgetController;
class ULRMenuWidgetController;
class ULRScreenWidget;
class ULRTransitionWidgetController;

/** Assembles independently authored Lost Runic screens and their presentation controllers. */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic HUD"))
class LOSTRUNIC_API ALRHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

	void InitializeForController(ALRPlayerController* playerController);
	void SetObservedCharacter(ALRCharacter* character);
	void ShowNarrative(bool bVisible);
	void ShowMenu(ELRScreenType screen, bool bVisible);
	void ShowTransition(bool bVisible);

	ULRScreenWidget* GetScreen(ELRScreenType screen) const;
	ULRScreenWidget* GetFocusableScreen(ELRInputMode inputMode) const;
	ULRDialogueWidgetController* GetDialogueController() const { return DialogueController; }
	ULRMenuWidgetController* GetMenuController() const { return MenuController; }
	ULRTransitionWidgetController* GetTransitionController() const { return TransitionController; }

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Widgets")
	TSubclassOf<ULRScreenWidget> HUDScreenClass;

	UPROPERTY(EditDefaultsOnly, Category = "Widgets")
	TSubclassOf<ULRScreenWidget> StateOverlayScreenClass;

	UPROPERTY(EditDefaultsOnly, Category = "Widgets")
	TSubclassOf<ULRScreenWidget> NarrativeScreenClass;

	UPROPERTY(EditDefaultsOnly, Category = "Widgets")
	TSubclassOf<ULRScreenWidget> JournalScreenClass;

	UPROPERTY(EditDefaultsOnly, Category = "Widgets")
	TSubclassOf<ULRScreenWidget> InventoryScreenClass;

	UPROPERTY(EditDefaultsOnly, Category = "Widgets")
	TSubclassOf<ULRScreenWidget> CollectiblesScreenClass;

	UPROPERTY(EditDefaultsOnly, Category = "Widgets")
	TSubclassOf<ULRScreenWidget> PauseScreenClass;

	UPROPERTY(EditDefaultsOnly, Category = "Widgets")
	TSubclassOf<ULRScreenWidget> SaveSlotsScreenClass;

	UPROPERTY(EditDefaultsOnly, Category = "Widgets")
	TSubclassOf<ULRScreenWidget> TransitionScreenClass;

private:
	void CreateScreens(ALRPlayerController* playerController);
	void CreateScreen(ALRPlayerController* playerController, ELRScreenType screen, TSubclassOf<ULRScreenWidget> screenClass);
	void SetScreenVisible(ELRScreenType screen, bool bVisible);
	void HideMenuScreens();

	UFUNCTION()
	void HandleNarrativePresentationChanged(FLRNarrativePresentation presentation);

	UFUNCTION()
	void HandleMenuScreenChanged(ELRScreenType previousScreen, ELRScreenType currentScreen);

	UFUNCTION()
	void HandleTransitionVisibilityChanged(bool bVisible);

	UPROPERTY(Transient)
	TMap<ELRScreenType, TObjectPtr<ULRScreenWidget>> ScreenWidgets;

	UPROPERTY(Transient)
	TObjectPtr<ULRDialogueWidgetController> DialogueController;

	UPROPERTY(Transient)
	TObjectPtr<ULRHUDWidgetController> HUDController;

	UPROPERTY(Transient)
	TObjectPtr<ULRMenuWidgetController> MenuController;

	UPROPERTY(Transient)
	TObjectPtr<ULRTransitionWidgetController> TransitionController;
};
