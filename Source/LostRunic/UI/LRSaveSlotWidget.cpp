#include "UI/LRSaveSlotWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Save/LRSaveFormatting.h"

void ULRSaveSlotWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SlotButton->OnClicked.AddDynamic(this, &ULRSaveSlotWidget::HandlePrimaryClicked);
}

void ULRSaveSlotWidget::NativeOnAddedToFocusPath(const FFocusEvent& inFocusEvent)
{
	Super::NativeOnAddedToFocusPath(inFocusEvent);
	if (View.SlotId.IsValid())
	{
		OnFocused.Broadcast(View.SlotId);
	}
}

void ULRSaveSlotWidget::ApplyView(const FLRSaveSlotView& view)
{
	View = view;
	SlotButton->SetIsEnabled(view.bCanLoad || view.bCanOverwrite || view.bCanDelete);
	if (SlotNameText)
	{
		SlotNameText->SetText(view.SlotDisplayText);
	}
	if (MapNameText)
	{
		MapNameText->SetText(view.bHasData ? view.MapDisplayName : FText::GetEmpty());
	}
	if (SavedAtText)
	{
		SavedAtText->SetText(view.bHasData
			? LRSaveFormatting::FormatSavedAtLocal(view.SavedAtUtc) : FText::GetEmpty());
	}
	if (PlayTimeText)
	{
		PlayTimeText->SetText(view.bHasData
			? LRSaveFormatting::FormatPlayTime(view.PlayTimeSeconds) : FText::GetEmpty());
	}
	if (CollectibleCountText)
	{
		CollectibleCountText->SetText(view.bHasData
			? LRSaveFormatting::FormatCollectedCount(view.CollectedCount, view.TotalCollectibleCount)
			: FText::GetEmpty());
	}
	if (HealthText)
	{
		HealthText->SetText(view.bHasData ? view.HealthDisplayText : FText::GetEmpty());
	}
	OnViewChanged(View);
}

bool ULRSaveSlotWidget::SetSlotFocus()
{
	if (!SlotButton || !SlotButton->GetIsEnabled())
	{
		return false;
	}
	SlotButton->SetUserFocus(GetOwningPlayer());
	return true;
}

void ULRSaveSlotWidget::HandlePrimaryClicked()
{
	if (View.SlotId.IsValid())
	{
		OnPrimaryRequested.Broadcast(View.SlotId);
	}
}
