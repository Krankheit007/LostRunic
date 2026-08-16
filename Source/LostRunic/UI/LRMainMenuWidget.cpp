#include "UI/LRMainMenuWidget.h"

#include "Components/Button.h"
#include "UI/LRMainMenuWidgetController.h"

void ULRMainMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (NewGameButton)
	{
		NewGameButton->OnClicked.AddDynamic(this, &ULRMainMenuWidget::HandleNewGameClicked);
	}
	if (ContinueButton)
	{
		ContinueButton->OnClicked.AddDynamic(this, &ULRMainMenuWidget::HandleContinueClicked);
	}
	if (LoadButton)
	{
		LoadButton->OnClicked.AddDynamic(this, &ULRMainMenuWidget::HandleLoadClicked);
	}
	if (ExitButton)
	{
		ExitButton->OnClicked.AddDynamic(this, &ULRMainMenuWidget::HandleExitClicked);
	}
}

void ULRMainMenuWidget::SetMainMenuWidgetController(ULRMainMenuWidgetController* controller)
{
	if (MainMenuWidgetController)
	{
		MainMenuWidgetController->OnViewChanged.RemoveDynamic(this, &ULRMainMenuWidget::HandleViewChanged);
	}
	MainMenuWidgetController = controller;
	if (MainMenuWidgetController)
	{
		MainMenuWidgetController->OnViewChanged.AddDynamic(this, &ULRMainMenuWidget::HandleViewChanged);
		HandleViewChanged(MainMenuWidgetController->GetViewModel());
	}
}

void ULRMainMenuWidget::NativeDestruct()
{
	SetMainMenuWidgetController(nullptr);
	Super::NativeDestruct();
}

void ULRMainMenuWidget::HandleViewChanged(const FLRMainMenuViewModel& viewModel)
{
	if (NewGameButton)
	{
		NewGameButton->SetIsEnabled(viewModel.bCanNewGame);
	}
	if (ContinueButton)
	{
		ContinueButton->SetIsEnabled(viewModel.bCanContinue);
	}
	if (LoadButton)
	{
		LoadButton->SetIsEnabled(viewModel.bCanLoad);
	}
	if (OptionsButton)
	{
		OptionsButton->SetIsEnabled(viewModel.bCanOptions);
	}
	if (ExitButton)
	{
		ExitButton->SetIsEnabled(viewModel.bCanExit);
	}
	OnMainMenuViewChanged(viewModel);
}

void ULRMainMenuWidget::HandleNewGameClicked()
{
	if (MainMenuWidgetController)
	{
		MainMenuWidgetController->RequestNewGame();
	}
}

void ULRMainMenuWidget::HandleContinueClicked()
{
	if (MainMenuWidgetController)
	{
		MainMenuWidgetController->RequestContinue();
	}
}

void ULRMainMenuWidget::HandleLoadClicked()
{
	OnLoadRequested.Broadcast();
}

void ULRMainMenuWidget::HandleExitClicked()
{
	if (MainMenuWidgetController)
	{
		MainMenuWidgetController->RequestExit();
	}
}

bool ULRMainMenuWidget::HandleUICommand_Implementation(const ELRUICommand command)
{
	if (command == ELRUICommand::Confirm || command == ELRUICommand::PrimaryAction)
	{
		return ExecuteFocusedAction();
	}
	return Super::HandleUICommand_Implementation(command);
}

bool ULRMainMenuWidget::SetInitialFocus()
{
	for (UButton* button : { NewGameButton.Get(), ContinueButton.Get(), LoadButton.Get(), ExitButton.Get() })
	{
		if (button && button->GetIsEnabled())
		{
			return SetFocusToWidget(button);
		}
	}
	return Super::SetInitialFocus();
}

bool ULRMainMenuWidget::ExecuteFocusedAction()
{
	if (NewGameButton->HasUserFocus(GetOwningPlayer())) HandleNewGameClicked();
	else if (ContinueButton->HasUserFocus(GetOwningPlayer())) HandleContinueClicked();
	else if (LoadButton->HasUserFocus(GetOwningPlayer())) HandleLoadClicked();
	else if (ExitButton->HasUserFocus(GetOwningPlayer())) HandleExitClicked();
	else return false;
	return true;
}
