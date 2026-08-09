#include "UI/LRHUD.h"

#include "Data/LRGameTuningSet.h"
#include "Framework/LRCharacter.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "Framework/LRPlayerController.h"
#include "Narrative/LRDialogueSubsystem.h"
#include "UI/LRDialogueWidgetController.h"
#include "UI/LRHUDWidgetController.h"
#include "UI/LRMenuWidgetController.h"
#include "UI/LRScreenWidget.h"
#include "UI/LRTransitionWidgetController.h"

void ALRHUD::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	if (DialogueController)
	{
		DialogueController->Deinitialize();
	}
	if (HUDController)
	{
		HUDController->Deinitialize();
	}
	Super::EndPlay(endPlayReason);
}

void ALRHUD::InitializeForController(ALRPlayerController* playerController)
{
	if (!playerController)
	{
		return;
	}
	CreateScreens(playerController);
	if (!DialogueController)
	{
		DialogueController = NewObject<ULRDialogueWidgetController>(this);
		ULRDialogueSubsystem* dialogueSubsystem = GetGameInstance()->GetSubsystem<ULRDialogueSubsystem>();
		ULRGameInstanceSubsystem* dataSubsystem = GetGameInstance()->GetSubsystem<ULRGameInstanceSubsystem>();
		ULRGameTuningSet* tuningSet = dataSubsystem ? dataSubsystem->GetTuningSet() : nullptr;
		DialogueController->Initialize(dialogueSubsystem, tuningSet ? tuningSet->UI : nullptr, GetWorld());
		DialogueController->OnPresentationChanged.AddDynamic(this, &ALRHUD::HandleNarrativePresentationChanged);

		HUDController = NewObject<ULRHUDWidgetController>(this);
		MenuController = NewObject<ULRMenuWidgetController>(this);
		MenuController->OnMenuScreenChanged.AddDynamic(this, &ALRHUD::HandleMenuScreenChanged);
		TransitionController = NewObject<ULRTransitionWidgetController>(this);
		TransitionController->OnTransitionVisibilityChanged.AddDynamic(this, &ALRHUD::HandleTransitionVisibilityChanged);
	}
	SetObservedCharacter(Cast<ALRCharacter>(playerController->GetPawn()));
	SetScreenVisible(ELRScreenType::HUD, true);
	SetScreenVisible(ELRScreenType::StateOverlay, true);
}

void ALRHUD::SetObservedCharacter(ALRCharacter* character)
{
	if (HUDController)
	{
		HUDController->SetObservedCharacter(character);
	}
}

void ALRHUD::ShowNarrative(const bool bVisible)
{
	SetScreenVisible(ELRScreenType::Narrative, bVisible);
}

void ALRHUD::ShowMenu(const ELRScreenType screen, const bool bVisible)
{
	if (!MenuController)
	{
		return;
	}
	if (bVisible && MenuController->OpenScreen(screen))
	{
		return;
	}
	if (!bVisible)
	{
		MenuController->CloseScreen();
	}
}

void ALRHUD::ShowTransition(const bool bVisible)
{
	if (TransitionController)
	{
		TransitionController->SetTransitionVisible(bVisible);
	}
}

ULRScreenWidget* ALRHUD::GetScreen(const ELRScreenType screen) const
{
	const TObjectPtr<ULRScreenWidget>* found = ScreenWidgets.Find(screen);
	return found ? found->Get() : nullptr;
}

ULRScreenWidget* ALRHUD::GetFocusableScreen(const ELRInputMode inputMode) const
{
	if (inputMode == ELRInputMode::Dialogue)
	{
		return GetScreen(ELRScreenType::Narrative);
	}
	if (inputMode == ELRInputMode::Menu && MenuController)
	{
		return GetScreen(MenuController->GetOpenScreen());
	}
	if (inputMode == ELRInputMode::Transition)
	{
		return GetScreen(ELRScreenType::Transition);
	}
	return nullptr;
}

void ALRHUD::CreateScreens(ALRPlayerController* playerController)
{
	if (!ScreenWidgets.IsEmpty())
	{
		return;
	}
	CreateScreen(playerController, ELRScreenType::HUD, HUDScreenClass);
	CreateScreen(playerController, ELRScreenType::StateOverlay, StateOverlayScreenClass);
	CreateScreen(playerController, ELRScreenType::Narrative, NarrativeScreenClass);
	CreateScreen(playerController, ELRScreenType::Journal, JournalScreenClass);
	CreateScreen(playerController, ELRScreenType::Inventory, InventoryScreenClass);
	CreateScreen(playerController, ELRScreenType::Collectibles, CollectiblesScreenClass);
	CreateScreen(playerController, ELRScreenType::Pause, PauseScreenClass);
	CreateScreen(playerController, ELRScreenType::SaveSlots, SaveSlotsScreenClass);
	CreateScreen(playerController, ELRScreenType::Transition, TransitionScreenClass);
}

void ALRHUD::CreateScreen(ALRPlayerController* playerController, const ELRScreenType screen,
	const TSubclassOf<ULRScreenWidget> screenClass)
{
	if (!screenClass)
	{
		return;
	}
	ULRScreenWidget* widget = CreateWidget<ULRScreenWidget>(playerController, screenClass);
	if (widget)
	{
		widget->AddToPlayerScreen();
		widget->SetScreenVisible(false);
		ScreenWidgets.Add(screen, widget);
	}
}

void ALRHUD::SetScreenVisible(const ELRScreenType screen, const bool bVisible)
{
	if (ULRScreenWidget* widget = GetScreen(screen))
	{
		widget->SetScreenVisible(bVisible);
	}
}

void ALRHUD::HideMenuScreens()
{
	SetScreenVisible(ELRScreenType::Journal, false);
	SetScreenVisible(ELRScreenType::Inventory, false);
	SetScreenVisible(ELRScreenType::Collectibles, false);
	SetScreenVisible(ELRScreenType::Pause, false);
	SetScreenVisible(ELRScreenType::SaveSlots, false);
}

void ALRHUD::HandleNarrativePresentationChanged(const FLRNarrativePresentation presentation)
{
	if (ULRScreenWidget* widget = GetScreen(ELRScreenType::Narrative))
	{
		widget->PresentNarrative(presentation);
	}
}

void ALRHUD::HandleMenuScreenChanged(const ELRScreenType previousScreen, const ELRScreenType currentScreen)
{
	HideMenuScreens();
	SetScreenVisible(currentScreen, currentScreen != ELRScreenType::None);
}

void ALRHUD::HandleTransitionVisibilityChanged(const bool bVisible)
{
	SetScreenVisible(ELRScreenType::Transition, bVisible);
}
