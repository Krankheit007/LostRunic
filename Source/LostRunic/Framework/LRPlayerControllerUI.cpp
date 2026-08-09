#include "Framework/LRPlayerController.h"

#include "Framework/LRCharacter.h"
#include "Interaction/LRInteractionComponent.h"
#include "Items/LRInventoryComponent.h"
#include "State/LRStateComponent.h"
#include "UI/LRHUD.h"
#include "UI/LRPlayerUIComponent.h"
#include "UI/LRScreenWidget.h"

void ALRPlayerController::OpenMenuScreen(const ELRScreenType screen)
{
	if (PlayerUI)
	{
		PlayerUI->OpenMenuScreen(screen);
	}
}

void ALRPlayerController::CloseMenuScreen()
{
	if (PlayerUI)
	{
		PlayerUI->CloseMenuScreen();
	}
}

FLRItemUseResult ALRPlayerController::UseInventoryItemFromMenu(const FName itemId)
{
	return PlayerUI ? PlayerUI->UseInventoryItem(itemId) : FLRItemUseResult();
}

void ALRPlayerController::HandleConfirm()
{
	if (PlayerUI)
	{
		PlayerUI->HandleConfirm();
	}
}

void ALRPlayerController::HandleCancel()
{
	if (PlayerUI)
	{
		PlayerUI->HandleCancel();
	}
}

void ALRPlayerController::HandleOpenJournal()
{
	if (PlayerUI)
	{
		PlayerUI->HandleOpenJournal();
	}
}

void ALRPlayerController::HandlePause()
{
	if (PlayerUI)
	{
		PlayerUI->HandlePause();
	}
}

void ALRPlayerController::UseQuickSlot(const int32 slotIndex)
{
	if (const ALRCharacter* character = Cast<ALRCharacter>(GetPawn()))
	{
		character->GetInventoryComponent()->UseQuickSlot(slotIndex,
			character->GetInteractionComponent()->GetCurrentTarget(), character->GetStateComponent()->GetCurrentMode());
	}
}

void ALRPlayerController::ConfigureViewportInput(const ELRInputMode newMode)
{
	if (newMode == ELRInputMode::Gameplay)
	{
		bShowMouseCursor = false;
		FInputModeGameOnly inputMode;
		SetInputMode(inputMode);
		return;
	}

	ALRHUD* hud = GetLRHUD();
	ULRScreenWidget* focusWidget = hud ? hud->GetFocusableScreen(newMode) : nullptr;
	if (newMode == ELRInputMode::Transition)
	{
		FInputModeUIOnly inputMode;
		if (focusWidget)
		{
			inputMode.SetWidgetToFocus(focusWidget->TakeWidget());
		}
		SetInputMode(inputMode);
		bShowMouseCursor = false;
		return;
	}

	FInputModeGameAndUI inputMode;
	inputMode.SetHideCursorDuringCapture(newMode != ELRInputMode::Menu);
	if (focusWidget)
	{
		inputMode.SetWidgetToFocus(focusWidget->TakeWidget());
	}
	SetInputMode(inputMode);
	bShowMouseCursor = newMode == ELRInputMode::Menu;
}

ALRHUD* ALRPlayerController::GetLRHUD() const
{
	return GetHUD<ALRHUD>();
}
