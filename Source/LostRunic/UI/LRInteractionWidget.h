/**
 * @file LRInteractionWidget.h
 * @brief Minimal visual widget for the interaction key and prompt text.
 */
#pragma once

#include "Blueprint/UserWidget.h"

#include "LRInteractionWidget.generated.h"

class UTextBlock;

/** Displays interaction text only; world projection and input resolution belong to its owners. */
UCLASS(Blueprintable, meta = (DisplayName = "Lost Runic Interaction Widget"))
class LOSTRUNIC_API ULRInteractionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Sets the already-resolved device key label. */
	void SetInteractionKey(const FText& key);

	/** Sets the localized interaction description. */
	void SetInteractionInfo(const FText& info);

	/** Applies the presentation visibility contract without enabling hit testing. */
	void SetPromptVisible(bool bVisible);

protected:
	/** Initializes the pure presentation widget as collapsed. */
	virtual void NativeOnInitialized() override;

	/** Explicit WBP_Interaction contract: a TextBlock named InteractionKey. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> InteractionKey;

	/** Explicit WBP_Interaction contract: a TextBlock named InteractionInfo. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> InteractionInfo;
};
