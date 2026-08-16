#include "UI/LRMainMenuHUD.h"

#include "Engine/GameInstance.h"
#include "Framework/LRPlayerController.h"
#include "Save/LRSaveSubsystem.h"
#include "UI/LRMainMenuWidget.h"
#include "UI/LRMenuWidgetController.h"
#include "UI/LRPlayerUIComponent.h"

void ALRMainMenuHUD::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	if (MainMenuWidgetController)
	{
		MainMenuWidgetController->Deinitialize();
	}
	if (MainMenuWidget)
	{
		MainMenuWidget->RemoveFromParent();
		MainMenuWidget = nullptr;
	}
	Super::EndPlay(endPlayReason);
}

void ALRMainMenuHUD::InitializeForController(ALRPlayerController* playerController)
{
	Super::InitializeForController(playerController);
	if (!MainMenuWidgetController && GetGameInstance())
	{
		MainMenuWidgetController = NewObject<ULRMainMenuWidgetController>(this);
		MainMenuWidgetController->Initialize(GetGameInstance()->GetSubsystem<ULRSaveSubsystem>());
	}
	if (!MainMenuWidget && MainMenuScreenClass)
	{
		MainMenuWidget = CreateWidget<ULRMainMenuWidget>(playerController, MainMenuScreenClass);
		if (MainMenuWidget)
		{
			MainMenuWidget->AddToPlayerScreen();
			MainMenuWidget->SetMainMenuWidgetController(MainMenuWidgetController);
			MainMenuWidget->OnLoadRequested.AddDynamic(this, &ALRMainMenuHUD::HandleLoadRequested);
			MainMenuWidget->SetScreenVisible(true);
		}
	}
	if (playerController->GetPlayerUI())
	{
		playerController->GetPlayerUI()->SetMenuLayer(true);
	}
}

ULRScreenWidget* ALRMainMenuHUD::GetFocusableScreen(const ELRInputMode inputMode) const
{
	if (inputMode == ELRInputMode::Menu)
	{
		if (ULRScreenWidget* saveScreen = GetScreen(ELRScreenType::SaveSlots);
			saveScreen && saveScreen->IsScreenVisible())
		{
			return saveScreen;
		}
		return MainMenuWidget;
	}
	return Super::GetFocusableScreen(inputMode);
}

void ALRMainMenuHUD::HandleLoadRequested()
{
	if (MainMenuWidget)
	{
		MainMenuWidget->SetScreenVisible(false);
	}
	OpenSaveSelection(ELRSaveSelectionMode::Load);
}

void ALRMainMenuHUD::ReturnFromSaveSelection()
{
	if (ULRMenuWidgetController* menuController = GetMenuController())
	{
		menuController->CloseScreen();
	}
	if (MainMenuWidget)
	{
		MainMenuWidget->SetScreenVisible(true);
		MainMenuWidget->RestoreFocus();
	}
}
