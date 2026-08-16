#include "UI/LRCreateSaveSlotWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

void ULRCreateSaveSlotWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (CreateButton)
	{
		CreateButton->OnClicked.AddDynamic(this, &ULRCreateSaveSlotWidget::HandleCreateClicked);
	}
}

void ULRCreateSaveSlotWidget::NativeOnAddedToFocusPath(const FFocusEvent& inFocusEvent)
{
	Super::NativeOnAddedToFocusPath(inFocusEvent);
	OnFocused.Broadcast();
}

void ULRCreateSaveSlotWidget::ApplyView(const int32 displayIndex, const FText& label)
{
	Slot_Index->SetText(FText::AsNumber(displayIndex));
	CreateLabelText->SetText(label);
}

bool ULRCreateSaveSlotWidget::SetCreateFocus()
{
	if (!CreateButton || !CreateButton->GetIsEnabled())
	{
		return false;
	}
	CreateButton->SetUserFocus(GetOwningPlayer());
	return true;
}

void ULRCreateSaveSlotWidget::HandleCreateClicked()
{
	OnCreateRequested.Broadcast();
}
