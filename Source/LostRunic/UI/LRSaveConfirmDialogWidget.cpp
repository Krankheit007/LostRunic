#include "UI/LRSaveConfirmDialogWidget.h"

#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"

void ULRSaveConfirmDialogWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	Cover_Confirm->OnClicked.AddDynamic(this, &ULRSaveConfirmDialogWidget::HandleConfirmClicked);
	Delete_Confirm->OnClicked.AddDynamic(this, &ULRSaveConfirmDialogWidget::HandleConfirmClicked);
	Cover_Cancel->OnClicked.AddDynamic(this, &ULRSaveConfirmDialogWidget::HandleCancelClicked);
	Delete_Cancel->OnClicked.AddDynamic(this, &ULRSaveConfirmDialogWidget::HandleCancelClicked);
}

void ULRSaveConfirmDialogWidget::ApplyViewModel(const FLRSaveConfirmViewModel& viewModel)
{
	ViewModel = viewModel;
	const bool bShowCover = ViewModel.bVisible && ViewModel.Confirmation == ELRSaveUIConfirmation::Overwrite;
	const bool bShowDelete = ViewModel.bVisible && ViewModel.Confirmation == ELRSaveUIConfirmation::Delete;
	Cover->SetVisibility(bShowCover ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	Delete->SetVisibility(bShowDelete ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	MessageText->SetText(ViewModel.Message);
	Cover_Confirm_T->SetText(ViewModel.ConfirmLabel);
	Cover_Cancel_T->SetText(ViewModel.CancelLabel);
	Delete_T_1->SetText(ViewModel.WarningMessage);
	Delete_T->SetText(ViewModel.Message);
	Delete_Confirm_T->SetText(ViewModel.ConfirmLabel);
	Delete_Cancel_T->SetText(ViewModel.CancelLabel);
	SetVisibility(bShowCover || bShowDelete ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	OnViewModelChanged(ViewModel);
	if (ViewModel.bVisible)
	{
		FocusDefaultAction();
	}
}

bool ULRSaveConfirmDialogWidget::FocusDefaultAction()
{
	UButton* target = ViewModel.Confirmation == ELRSaveUIConfirmation::Overwrite
		? Cover_Confirm.Get() : Delete_Confirm.Get();
	if (!target || !target->GetIsEnabled())
	{
		return false;
	}
	target->SetUserFocus(GetOwningPlayer());
	return true;
}

void ULRSaveConfirmDialogWidget::HandleConfirmClicked()
{
	OnConfirmRequested.Broadcast();
}

void ULRSaveConfirmDialogWidget::HandleCancelClicked()
{
	OnCancelRequested.Broadcast();
}
