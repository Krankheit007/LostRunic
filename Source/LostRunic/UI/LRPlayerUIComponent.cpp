#include "UI/LRPlayerUIComponent.h"

#include "Framework/LRCharacter.h"
#include "Framework/LRPlayerController.h"
#include "Interaction/LRInteractionComponent.h"
#include "Items/LRInventoryComponent.h"
#include "Narrative/LRDialogueSubsystem.h"
#include "State/LRStateComponent.h"
#include "UI/LRDialogueWidgetController.h"
#include "UI/LRHUD.h"

ULRPlayerUIComponent::ULRPlayerUIComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULRPlayerUIComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	UnbindNarrative();
	OwnerController.Reset();
	Super::EndPlay(endPlayReason);
}

void ULRPlayerUIComponent::InitializeUI(ALRPlayerController* playerController)
{
	if (!playerController || OwnerController == playerController)
	{
		return;
	}
	UnbindNarrative();
	OwnerController = playerController;
	DialogueSubsystem = playerController->GetGameInstance()->GetSubsystem<ULRDialogueSubsystem>();
	if (ULRDialogueSubsystem* dialogueSubsystem = DialogueSubsystem.Get())
	{
		dialogueSubsystem->OnPageChanged.AddDynamic(this, &ULRPlayerUIComponent::HandleNarrativePageChanged);
		dialogueSubsystem->OnSessionEnded.AddDynamic(this, &ULRPlayerUIComponent::HandleNarrativeSessionEnded);
	}
	if (ALRHUD* hud = GetLRHUD())
	{
		hud->InitializeForController(playerController);
	}
}

void ULRPlayerUIComponent::SetObservedCharacter(ALRCharacter* character)
{
	if (ALRHUD* hud = GetLRHUD())
	{
		hud->SetObservedCharacter(character);
	}
}

void ULRPlayerUIComponent::HandleConfirm()
{
	const ALRPlayerController* controller = OwnerController.Get();
	if (!controller || controller->GetLRInputMode() != ELRInputMode::Dialogue)
	{
		return;
	}
	if (ALRHUD* hud = GetLRHUD())
	{
		if (ULRDialogueWidgetController* dialogueController = hud->GetDialogueController())
		{
			dialogueController->HandleConfirm();
		}
	}
}

void ULRPlayerUIComponent::HandleCancel()
{
	const ALRPlayerController* controller = OwnerController.Get();
	if (!controller)
	{
		return;
	}
	if (controller->GetLRInputMode() == ELRInputMode::Dialogue)
	{
		if (ALRHUD* hud = GetLRHUD())
		{
			if (ULRDialogueWidgetController* dialogueController = hud->GetDialogueController())
			{
				dialogueController->EndSession();
			}
		}
	}
	else if (controller->GetLRInputMode() == ELRInputMode::Menu)
	{
		CloseMenuScreen();
	}
}

void ULRPlayerUIComponent::HandleOpenJournal()
{
	if (const ALRPlayerController* controller = OwnerController.Get())
	{
		if (controller->GetLRInputMode() == ELRInputMode::Gameplay)
		{
			OpenMenuScreen(ELRScreenType::Journal);
		}
		else if (controller->GetLRInputMode() == ELRInputMode::Menu)
		{
			CloseMenuScreen();
		}
	}
}

void ULRPlayerUIComponent::HandlePause()
{
	if (const ALRPlayerController* controller = OwnerController.Get())
	{
		if (controller->GetLRInputMode() == ELRInputMode::Gameplay)
		{
			OpenMenuScreen(ELRScreenType::Pause);
		}
		else if (controller->GetLRInputMode() == ELRInputMode::Menu)
		{
			CloseMenuScreen();
		}
	}
}

void ULRPlayerUIComponent::OpenMenuScreen(const ELRScreenType screen)
{
	ALRPlayerController* controller = OwnerController.Get();
	if (!controller || controller->GetLRInputMode() == ELRInputMode::Dialogue || screen == ELRScreenType::None)
	{
		return;
	}
	if (ALRHUD* hud = GetLRHUD())
	{
		hud->ShowMenu(screen, true);
		controller->SetLRInputMode(ELRInputMode::Menu);
	}
}

void ULRPlayerUIComponent::CloseMenuScreen()
{
	ALRPlayerController* controller = OwnerController.Get();
	if (!controller || controller->GetLRInputMode() != ELRInputMode::Menu)
	{
		return;
	}
	if (ALRHUD* hud = GetLRHUD())
	{
		hud->ShowMenu(ELRScreenType::None, false);
	}
	controller->SetLRInputMode(ELRInputMode::Gameplay);
}

FLRItemUseResult ULRPlayerUIComponent::UseInventoryItem(const FName itemId) const
{
	const ALRPlayerController* controller = OwnerController.Get();
	const ALRCharacter* character = controller ? Cast<ALRCharacter>(controller->GetPawn()) : nullptr;
	if (!character)
	{
		return FLRItemUseResult();
	}
	return character->GetInventoryComponent()->UseItemFromSelector(itemId,
		character->GetInteractionComponent()->GetCurrentTarget(), character->GetStateComponent()->GetCurrentMode());
}

void ULRPlayerUIComponent::HandleNarrativePageChanged(const FLRNarrativePage page)
{
	if (ALRHUD* hud = GetLRHUD())
	{
		hud->ShowNarrative(true);
	}
	if (ALRPlayerController* controller = OwnerController.Get())
	{
		controller->SetLRInputMode(ELRInputMode::Dialogue);
	}
}

void ULRPlayerUIComponent::HandleNarrativeSessionEnded(const ELRNarrativeSessionType sessionType, const FName finalContentId)
{
	if (ALRHUD* hud = GetLRHUD())
	{
		hud->ShowNarrative(false);
	}
	if (ALRPlayerController* controller = OwnerController.Get(); controller && controller->GetLRInputMode() == ELRInputMode::Dialogue)
	{
		controller->SetLRInputMode(ELRInputMode::Gameplay);
	}
}

ALRHUD* ULRPlayerUIComponent::GetLRHUD() const
{
	const ALRPlayerController* controller = OwnerController.Get();
	return controller ? controller->GetHUD<ALRHUD>() : nullptr;
}

void ULRPlayerUIComponent::UnbindNarrative()
{
	if (ULRDialogueSubsystem* dialogueSubsystem = DialogueSubsystem.Get())
	{
		dialogueSubsystem->OnPageChanged.RemoveDynamic(this, &ULRPlayerUIComponent::HandleNarrativePageChanged);
		dialogueSubsystem->OnSessionEnded.RemoveDynamic(this, &ULRPlayerUIComponent::HandleNarrativeSessionEnded);
	}
	DialogueSubsystem.Reset();
}
