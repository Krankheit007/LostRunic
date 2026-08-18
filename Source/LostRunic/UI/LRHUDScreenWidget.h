/**
 * @file LRHUDScreenWidget.h
 * @brief Projects the controller-owned interaction prompt from a world anchor onto the HUD canvas.
 */
#pragma once

#include "Interaction/LRInteractionTypes.h"
#include "Layout/Geometry.h"
#include "UI/LRScreenWidget.h"

#include "LRHUDScreenWidget.generated.h"

class ULRInteractionWidget;

/** HUD screen responsible for prompt placement and viewport visibility only. */
UCLASS(Blueprintable, meta = (DisplayName = "Lost Runic HUD Screen Widget"))
class LOSTRUNIC_API ULRHUDScreenWidget : public ULRScreenWidget
{
	GENERATED_BODY()

public:
	/** Injects the HUD controller and subscribes to its current prompt state. */
	virtual void SetHUDWidgetController(ULRHUDWidgetController* controller) override;

protected:
	/** Initializes the explicit WBP_HUD interaction child contract. */
	virtual void NativeOnInitialized() override;
	/** Projects a valid prompt each frame while its source remains valid. */
	virtual void NativeTick(const FGeometry& geometry, float deltaTime) override;
	/** Removes the HUD controller delegate before destruction. */
	virtual void NativeDestruct() override;

private:
	/** Applies controller-owned content and lifecycle state to the pure interaction widget. */
	UFUNCTION()
	void HandleInteractionPromptChanged(FLRInteractionPromptView promptView);

	/** Projects the weak anchor and applies the full-widget viewport test. */
	void UpdateProjectedPrompt();

	/** Explicit WBP_HUD contract: direct child named InteractionWidget under the Background Canvas. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULRInteractionWidget> InteractionWidget;

	FLRInteractionPromptView CurrentPrompt;
	bool bHasValidPrompt = false;
	bool bPresentationAllowed = false;
};
