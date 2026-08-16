#include "UI/LRSaveSelectionWidget.h"

#include "Core/LRLog.h"
#include "UI/LRCreateSaveSlotWidget.h"
#include "UI/LRSaveConfirmDialogWidget.h"
#include "UI/LRSaveSlotWidget.h"
#include "UI/LRSaveWidgetController.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

void ULRSaveSelectionWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BackButton->OnClicked.AddDynamic(this, &ULRSaveSelectionWidget::HandleBackClicked);
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
	if (ConfirmDialog)
	{
		ConfirmDialog->RemoveFromParent();
		ConfirmDialog = nullptr;
	}
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
		HandleBackClicked();
		return true;
	}
	else if (command == ELRUICommand::PrimaryAction)
	{
		if (snapshot.FocusTarget.Kind == ELRSaveFocusTargetKind::ExistingSlot)
		{
			SaveWidgetController->RequestPrimarySlotAction(snapshot.FocusTarget.SlotId);
			return true;
		}
		if (snapshot.FocusTarget.Kind == ELRSaveFocusTargetKind::CreateSlot)
		{
			SaveWidgetController->RequestCreateManualSave();
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
	// Treat the dynamic delegate as a refresh notification. Pulling the controller-owned
	// snapshot avoids losing nested FText values while the delegate payload is marshalled.
	const FLRSaveUISnapshot& currentSnapshot = SaveWidgetController
		? SaveWidgetController->GetSnapshot()
		: snapshot;
	RebuildSlotRows(currentSnapshot);
	TitleText->SetText(currentSnapshot.Title);
	StatusText->SetText(currentSnapshot.StatusMessage);
	EnsureConfirmDialog();
	if (ConfirmDialog)
	{
		ConfirmDialog->ApplyViewModel(currentSnapshot.ConfirmationViewModel);
	}
	OnSaveSnapshotChanged(currentSnapshot);
	if (IsScreenVisible() && !currentSnapshot.ConfirmationViewModel.bVisible)
	{
		RestoreFocus();
	}
}

void ULRSaveSelectionWidget::HandleSlotPrimaryRequested(const FLRSaveSlotId slotId)
{
	if (SaveWidgetController)
	{
		SaveWidgetController->RequestPrimarySlotAction(slotId);
	}
}

void ULRSaveSelectionWidget::HandleSlotFocused(const FLRSaveSlotId slotId)
{
	if (SaveWidgetController)
	{
		SaveWidgetController->UpdateFocusTarget(FLRSaveFocusTarget::MakeExisting(slotId));
	}
}

void ULRSaveSelectionWidget::HandleCreateSlotClicked()
{
	if (SaveWidgetController)
	{
		SaveWidgetController->RequestCreateManualSave();
	}
}

void ULRSaveSelectionWidget::HandleCreateSlotFocused()
{
	if (SaveWidgetController)
	{
		SaveWidgetController->UpdateFocusTarget(
			FLRSaveFocusTarget::MakeCreate(SaveWidgetController->GetSnapshot().CreateDisplayIndex));
	}
}

void ULRSaveSelectionWidget::HandleConfirmRequested()
{
	if (SaveWidgetController)
	{
		SaveWidgetController->ConfirmPendingAction();
	}
}

void ULRSaveSelectionWidget::HandleCancelRequested()
{
	if (SaveWidgetController)
	{
		SaveWidgetController->CancelPendingAction();
	}
}

void ULRSaveSelectionWidget::HandleBackClicked()
{
	OnBackRequested.Broadcast();
}

void ULRSaveSelectionWidget::RebuildSlotRows(const FLRSaveUISnapshot& snapshot)
{
	SlotListPanel->ClearChildren();
	SlotRows.Reset();
	CreateSlotRow = nullptr;
	for (const FLRSaveSlotView& view : snapshot.Slots)
	{
		ULRSaveSlotWidget* row = CreateWidget<ULRSaveSlotWidget>(GetOwningPlayer(), SaveSlotWidgetClass);
		if (!row)
		{
			UE_LOG(LogLostRunicUI, Warning, TEXT("Save slot row creation failed; slot is skipped."));
			continue;
		}
		row->ApplyView(view);
		row->OnPrimaryRequested.AddDynamic(this, &ULRSaveSelectionWidget::HandleSlotPrimaryRequested);
		row->OnFocused.AddDynamic(this, &ULRSaveSelectionWidget::HandleSlotFocused);
		AddSlotRow(row);
		SlotRows.Add(view.SlotId, row);
	}
	if (snapshot.bCanCreateManualSlot)
	{
		CreateSlotRow = CreateWidget<ULRCreateSaveSlotWidget>(GetOwningPlayer(), CreateSaveSlotWidgetClass);
		if (CreateSlotRow)
		{
			CreateSlotRow->ApplyView(snapshot.CreateDisplayIndex, snapshot.CreateLabel);
			CreateSlotRow->OnCreateRequested.AddDynamic(this, &ULRSaveSelectionWidget::HandleCreateSlotClicked);
			CreateSlotRow->OnFocused.AddDynamic(this, &ULRSaveSelectionWidget::HandleCreateSlotFocused);
			AddSlotRow(CreateSlotRow);
		}
	}
}

void ULRSaveSelectionWidget::AddSlotRow(UWidget* row)
{
	if (!row || !WidgetTree)
	{
		return;
	}
	// 行高度由 WBP_SaveSlot / WBP_CreateSaveSlot 根画布自身声明（设计器 200 单元）。
	// 不再用运行时构造的 SizeBox 包装：运行时容器与行内嵌套画布的期望尺寸计算
	// 相互干扰，会导致行内文本分支在布局时被排除（0x0 几何）。
	SlotListPanel->AddChild(row);
}

void ULRSaveSelectionWidget::EnsureConfirmDialog()
{
	if (ConfirmDialog || !ConfirmDialogWidgetClass)
	{
		return;
	}
	ConfirmDialog = CreateWidget<ULRSaveConfirmDialogWidget>(GetOwningPlayer(), ConfirmDialogWidgetClass);
	if (ConfirmDialog)
	{
		ConfirmDialog->AddToPlayerScreen(20);
		ConfirmDialog->OnConfirmRequested.AddDynamic(this, &ULRSaveSelectionWidget::HandleConfirmRequested);
		ConfirmDialog->OnCancelRequested.AddDynamic(this, &ULRSaveSelectionWidget::HandleCancelRequested);
	}
}

bool ULRSaveSelectionWidget::SetInitialFocus()
{
	return RestoreFocus();
}

bool ULRSaveSelectionWidget::RestoreFocus()
{
	if (!SaveWidgetController)
	{
		return Super::RestoreFocus();
	}
	const FLRSaveFocusTarget focus = SaveWidgetController->GetFocusTarget();
	if (focus.Kind == ELRSaveFocusTargetKind::ExistingSlot)
	{
		if (const TObjectPtr<ULRSaveSlotWidget>* row = SlotRows.Find(focus.SlotId))
		{
			return (*row)->SetSlotFocus();
		}
	}
	if (focus.Kind == ELRSaveFocusTargetKind::CreateSlot && CreateSlotRow)
	{
		return CreateSlotRow->SetCreateFocus();
	}
	return SetFocusToWidget(BackButton);
}

void ULRSaveSelectionWidget::SetScreenVisible(const bool bVisible)
{
	Super::SetScreenVisible(bVisible);
	if (bVisible)
	{
		RestoreFocus();
	}
}

void ULRSaveSelectionWidget::UnbindSaveController()
{
	if (SaveWidgetController)
	{
		SaveWidgetController->OnSnapshotChanged.RemoveDynamic(this, &ULRSaveSelectionWidget::HandleSaveSnapshotChanged);
	}
}
