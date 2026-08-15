#include "UI/LRSaveSelectionWidget.h"

#include "UI/LRSaveWidgetController.h"

#include "Components/Button.h"

void ULRSaveSelectionWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (CreateSlotButton)
	{
		CreateSlotButton->OnClicked.AddDynamic(this, &ULRSaveSelectionWidget::HandleCreateSlotClicked);
	}
	if (BackButton)
	{
		BackButton->OnClicked.AddDynamic(this, &ULRSaveSelectionWidget::HandleBackClicked);
	}
}

void ULRSaveSelectionWidget::SetSaveWidgetController(ULRSaveWidgetController* controller)
{
	UnbindSaveController();
	Super::SetSaveWidgetController(controller);
	if (controller)
	{
		controller->OnSnapshotChanged.AddDynamic(this, &ULRSaveSelectionWidget::HandleSaveSnapshotChanged);
		HandleSaveSnapshotChanged(controller->GetSnapshot());
	}
}

void ULRSaveSelectionWidget::NativeDestruct()
{
	UnbindSaveController();
	Super::NativeDestruct();
}

bool ULRSaveSelectionWidget::HandleUICommand_Implementation(const ELRUICommand command)
{
	if (!SaveWidgetController)
	{
		return false;
	}
	const FLRSaveUISnapshot& snapshot = SaveWidgetController->GetSnapshot();
	if (command == ELRUICommand::Confirm)
	{
		if (snapshot.State == ELRSaveUIState::Confirming)
		{
			SaveWidgetController->ConfirmPendingAction();
			return true;
		}
	}
	else if (command == ELRUICommand::Cancel)
	{
		if (snapshot.State == ELRSaveUIState::Confirming)
		{
			SaveWidgetController->CancelPendingAction();
			return true;
		}
		if (snapshot.State == ELRSaveUIState::Error)
		{
			SaveWidgetController->DismissError();
			return true;
		}
	}
	else if (command == ELRUICommand::Delete
		&& snapshot.FocusTarget.Kind == ELRSaveFocusTargetKind::ExistingSlot)
	{
		SaveWidgetController->RequestDelete(snapshot.FocusTarget.SlotId);
		return true;
	}
	return Super::HandleUICommand_Implementation(command);
}

void ULRSaveSelectionWidget::HandleSaveSnapshotChanged(const FLRSaveUISnapshot& snapshot)
{
	OnSaveSnapshotChanged(snapshot);
}

void ULRSaveSelectionWidget::HandleCreateSlotClicked()
{
	if (SaveWidgetController)
	{
		SaveWidgetController->RequestCreateManualSave();
	}
}

void ULRSaveSelectionWidget::HandleBackClicked()
{
	OnBackRequested();
}

void ULRSaveSelectionWidget::UnbindSaveController()
{
	if (SaveWidgetController)
	{
		SaveWidgetController->OnSnapshotChanged.RemoveDynamic(this, &ULRSaveSelectionWidget::HandleSaveSnapshotChanged);
	}
}
