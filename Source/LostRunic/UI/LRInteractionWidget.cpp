/**
 * @file LRInteractionWidget.cpp
 * @brief Minimal visual widget for the interaction key and prompt text.
 */
#include "UI/LRInteractionWidget.h"

#include "Components/TextBlock.h"

void ULRInteractionWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetPromptVisible(false);
}

void ULRInteractionWidget::SetInteractionKey(const FText& key)
{
	if (InteractionKey)
	{
		InteractionKey->SetText(key);
	}
}

void ULRInteractionWidget::SetInteractionInfo(const FText& info)
{
	if (InteractionInfo)
	{
		InteractionInfo->SetText(info);
	}
}

void ULRInteractionWidget::SetPromptVisible(const bool bVisible)
{
	SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}
