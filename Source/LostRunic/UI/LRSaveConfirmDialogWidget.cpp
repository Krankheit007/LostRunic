#include "UI/LRSaveConfirmDialogWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

void ULRSaveConfirmDialogWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (ConfirmButton)
	{
		ConfirmButton->OnClicked.AddDynamic(this, &ULRSaveConfirmDialogWidget::HandleConfirmClicked);
	}
	if (CancelButton)
	{
		CancelButton->OnClicked.AddDynamic(this, &ULRSaveConfirmDialogWidget::HandleCancelClicked);
	}
}

void ULRSaveConfirmDialogWidget::ApplyViewModel(const FLRSaveConfirmViewModel& viewModel)
{
	ViewModel = viewModel;
	SetVisibility(ViewModel.bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (MessageText)
	{
		MessageText->SetText(ViewModel.Message);
	}
	OnViewModelChanged(ViewModel);
}

void ULRSaveConfirmDialogWidget::HandleConfirmClicked()
{
	OnConfirmRequested.Broadcast();
}

void ULRSaveConfirmDialogWidget::HandleCancelClicked()
{
	OnCancelRequested.Broadcast();
}
