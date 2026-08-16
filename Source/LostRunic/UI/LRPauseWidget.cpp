#include "UI/LRPauseWidget.h"

#include "Components/Button.h"

void ULRPauseWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	Resume->OnClicked.AddDynamic(this, &ULRPauseWidget::HandleResumeClicked);
	SaveGame->OnClicked.AddDynamic(this, &ULRPauseWidget::HandleSaveClicked);
	MainMenu->OnClicked.AddDynamic(this, &ULRPauseWidget::HandleMainMenuClicked);
	Options->SetIsEnabled(false);
}

bool ULRPauseWidget::HandleUICommand_Implementation(const ELRUICommand command)
{
	if (command == ELRUICommand::Cancel)
	{
		HandleResumeClicked();
		return true;
	}
	if (command == ELRUICommand::Confirm || command == ELRUICommand::PrimaryAction)
	{
		return ExecuteFocusedAction();
	}
	return Super::HandleUICommand_Implementation(command);
}

bool ULRPauseWidget::SetInitialFocus()
{
	return SetFocusToWidget(Resume);
}

void ULRPauseWidget::HandleResumeClicked()
{
	OnResumeRequested.Broadcast();
}

void ULRPauseWidget::HandleSaveClicked()
{
	OnSaveRequested.Broadcast();
}

void ULRPauseWidget::HandleMainMenuClicked()
{
	OnMainMenuRequested.Broadcast();
}

bool ULRPauseWidget::ExecuteFocusedAction()
{
	if (Resume->HasUserFocus(GetOwningPlayer()))
	{
		HandleResumeClicked();
		return true;
	}
	if (SaveGame->HasUserFocus(GetOwningPlayer()))
	{
		HandleSaveClicked();
		return true;
	}
	if (MainMenu->HasUserFocus(GetOwningPlayer()))
	{
		HandleMainMenuClicked();
		return true;
	}
	return false;
}
