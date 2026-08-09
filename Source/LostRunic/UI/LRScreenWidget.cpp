#include "UI/LRScreenWidget.h"

void ULRScreenWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::Collapsed);
}

void ULRScreenWidget::SetScreenVisible(const bool bVisible)
{
	bScreenVisible = bVisible;
	SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	OnScreenVisibilityChanged(bVisible);
}

void ULRScreenWidget::PresentNarrative(const FLRNarrativePresentation& presentation)
{
	OnNarrativePresentationChanged(presentation);
}
