#include "UI/LRCreateSaveSlotWidget.h"

#include "Components/Button.h"

void ULRCreateSaveSlotWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (CreateButton)
	{
		CreateButton->OnClicked.AddDynamic(this, &ULRCreateSaveSlotWidget::HandleCreateClicked);
	}
}

void ULRCreateSaveSlotWidget::HandleCreateClicked()
{
	OnCreateRequested.Broadcast();
}
