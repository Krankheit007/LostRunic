/**
 * @file LRHUDScreenWidget.cpp
 * @brief Projects the controller-owned interaction prompt from a world anchor onto the HUD canvas.
 */
#include "UI/LRHUDScreenWidget.h"

#include "Blueprint/SlateBlueprintLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelWidget.h"
#include "UI/LRHUDWidgetController.h"
#include "UI/LRInteractionWidget.h"

void ULRHUDScreenWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (InteractionWidget)
	{
		if (UCanvasPanelSlot* canvasSlot = Cast<UCanvasPanelSlot>(InteractionWidget->Slot))
		{
			canvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
			canvasSlot->SetAutoSize(true);
			canvasSlot->SetAlignment(FVector2D(0.5f, 1.0f));
		}
		InteractionWidget->SetPromptVisible(false);
	}
}

void ULRHUDScreenWidget::SetHUDWidgetController(ULRHUDWidgetController* controller)
{
	if (HUDWidgetController)
	{
		HUDWidgetController->OnInteractionPromptChanged.RemoveDynamic(
			this, &ULRHUDScreenWidget::HandleInteractionPromptChanged);
	}

	Super::SetHUDWidgetController(controller);

	if (HUDWidgetController)
	{
		HUDWidgetController->OnInteractionPromptChanged.AddDynamic(
			this, &ULRHUDScreenWidget::HandleInteractionPromptChanged);
		HandleInteractionPromptChanged(HUDWidgetController->GetCurrentInteractionPrompt());
	}
	else
	{
		HandleInteractionPromptChanged(FLRInteractionPromptView());
	}
}

void ULRHUDScreenWidget::NativeTick(const FGeometry& geometry, const float deltaTime)
{
	Super::NativeTick(geometry, deltaTime);
	if (bHasValidPrompt && bPresentationAllowed && CurrentPrompt.PromptAnchor.IsValid())
	{
		UpdateProjectedPrompt();
	}
}

void ULRHUDScreenWidget::NativeDestruct()
{
	if (HUDWidgetController)
	{
		HUDWidgetController->OnInteractionPromptChanged.RemoveDynamic(
			this, &ULRHUDScreenWidget::HandleInteractionPromptChanged);
	}

	CurrentPrompt = FLRInteractionPromptView();
	bHasValidPrompt = false;
	bPresentationAllowed = false;
	Super::NativeDestruct();
}

void ULRHUDScreenWidget::HandleInteractionPromptChanged(const FLRInteractionPromptView promptView)
{
	CurrentPrompt = promptView;
	bHasValidPrompt = CurrentPrompt.Target.IsValid();
	bPresentationAllowed = CurrentPrompt.bVisible;

	if (!InteractionWidget)
	{
		return;
	}

	InteractionWidget->SetInteractionKey(CurrentPrompt.InputKeyText);
	InteractionWidget->SetInteractionInfo(CurrentPrompt.Prompt);
	InteractionWidget->SetPromptVisible(false);
	if (!bHasValidPrompt || !bPresentationAllowed || !CurrentPrompt.PromptAnchor.IsValid())
	{
		return;
	}
	UpdateProjectedPrompt();
}

void ULRHUDScreenWidget::UpdateProjectedPrompt()
{
	if (!InteractionWidget || !CurrentPrompt.PromptAnchor.IsValid())
	{
		return;
	}

	APlayerController* playerController = GetOwningPlayer();
	if (!playerController)
	{
		InteractionWidget->SetPromptVisible(false);
		return;
	}

	FVector2D screenPosition;
	const FVector worldPosition = CurrentPrompt.PromptAnchor->GetComponentLocation() + CurrentPrompt.PromptWorldOffset;
	if (!playerController->ProjectWorldLocationToScreen(worldPosition, screenPosition, true))
	{
		InteractionWidget->SetPromptVisible(false);
		return;
	}

	const UWidget* canvasWidget = InteractionWidget->GetParent();
	const FGeometry& canvasGeometry = canvasWidget ? canvasWidget->GetCachedGeometry() : GetCachedGeometry();
	FVector2D widgetPosition;
	USlateBlueprintLibrary::ScreenToWidgetLocal(this, canvasGeometry, screenPosition, widgetPosition);

	UCanvasPanelSlot* canvasSlot = Cast<UCanvasPanelSlot>(InteractionWidget->Slot);
	if (!canvasSlot)
	{
		InteractionWidget->SetPromptVisible(false);
		return;
	}

	canvasSlot->SetAlignment(FVector2D(0.5f, 1.0f));
	canvasSlot->SetPosition(widgetPosition);
	const FVector2D desiredSize = InteractionWidget->GetDesiredSize();
	const FVector2D viewportSize = canvasGeometry.GetLocalSize();
	const float left = widgetPosition.X - desiredSize.X * 0.5f;
	const float right = widgetPosition.X + desiredSize.X * 0.5f;
	const float top = widgetPosition.Y - desiredSize.Y;
	const float bottom = widgetPosition.Y;
	const bool bInsideViewport = left >= 0.0f && right <= viewportSize.X
		&& top >= 0.0f && bottom <= viewportSize.Y;
	InteractionWidget->SetPromptVisible(bInsideViewport);
}
