#include "UI/LRSaveSlotWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Save/LRSaveFormatting.h"

namespace
{
	FText HealthText(const ELRSaveSlotHealth health)
	{
		switch (health)
		{
		case ELRSaveSlotHealth::Healthy: return NSLOCTEXT("LRSaveUI", "HealthHealthy", "Healthy");
		case ELRSaveSlotHealth::MissingPayload: return NSLOCTEXT("LRSaveUI", "HealthMissing", "Missing");
		case ELRSaveSlotHealth::CorruptPayload: return NSLOCTEXT("LRSaveUI", "HealthCorrupt", "Corrupt");
		case ELRSaveSlotHealth::UnsupportedVersion: return NSLOCTEXT("LRSaveUI", "HealthUnsupported", "Unsupported");
		case ELRSaveSlotHealth::CatalogMismatch: return NSLOCTEXT("LRSaveUI", "HealthMismatch", "Mismatched");
		case ELRSaveSlotHealth::UnknownSlotType: return NSLOCTEXT("LRSaveUI", "HealthUnknownType", "Unknown");
		case ELRSaveSlotHealth::InvalidData: return NSLOCTEXT("LRSaveUI", "HealthInvalid", "Invalid");
		default: return NSLOCTEXT("LRSaveUI", "HealthInvalidFallback", "Invalid");
		}
	}
}

void ULRSaveSlotWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (PrimaryButton)
	{
		PrimaryButton->OnClicked.AddDynamic(this, &ULRSaveSlotWidget::HandlePrimaryClicked);
	}
	if (DeleteButton)
	{
		DeleteButton->OnClicked.AddDynamic(this, &ULRSaveSlotWidget::HandleDeleteClicked);
	}
}

void ULRSaveSlotWidget::ApplyView(const FLRSaveSlotView& view)
{
	View = view;
	if (PrimaryButton)
	{
		PrimaryButton->SetIsEnabled(view.bCanLoad || view.bCanOverwrite);
	}
	if (DeleteButton)
	{
		DeleteButton->SetIsEnabled(view.bCanDelete);
	}
	if (SlotNameText)
	{
		SlotNameText->SetText(FText::Format(NSLOCTEXT("LRSaveUI", "SlotName", "Slot {0}"),
			FText::AsNumber(view.DisplayIndex)));
	}
	if (MapNameText)
	{
		MapNameText->SetText(view.MapDisplayName);
	}
	if (SavedAtText)
	{
		SavedAtText->SetText(LRSaveFormatting::FormatSavedAtLocal(view.SavedAtUtc));
	}
	if (PlayTimeText)
	{
		PlayTimeText->SetText(LRSaveFormatting::FormatPlayTime(view.PlayTimeSeconds));
	}
	if (CollectibleCountText)
	{
		CollectibleCountText->SetText(LRSaveFormatting::FormatCollectedCount(view.CollectedCount,
			view.TotalCollectibleCount));
	}
	if (HealthText)
	{
		HealthText->SetText(::HealthText(view.Health));
	}
	OnViewChanged(View);
}

void ULRSaveSlotWidget::HandlePrimaryClicked()
{
	if (View.SlotId.IsValid())
	{
		OnPrimaryRequested.Broadcast(View.SlotId);
	}
}

void ULRSaveSlotWidget::HandleDeleteClicked()
{
	if (View.SlotId.IsValid())
	{
		OnDeleteRequested.Broadcast(View.SlotId);
	}
}
