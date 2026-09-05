/**
 * @file LRStateEyeOverlayWidget.cpp
 * @brief Presents the eye-hold close/open transition without owning state timing or gameplay completion.
 */
#include "UI/LRStateEyeOverlayWidget.h"

#include "Components/Image.h"
#include "UI/LRHUDWidgetController.h"

namespace
{
	constexpr float FullyOpenClosedness = 0.0f;
	constexpr float FullyClosedClosedness = 1.0f;
	constexpr float MinInterpolationSeconds = 0.001f;

	float EaseInOut(const float alpha)
	{
		return FMath::InterpEaseInOut(0.0f, 1.0f, FMath::Clamp(alpha, 0.0f, 1.0f), 2.0f);
	}
}

void ULRStateEyeOverlayWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetIsFocusable(false);
	bScreenLayerVisible = false;
	StableClosedness = FullyOpenClosedness;
	StableOpacity = 0.0f;
	StableTint = FLinearColor::Black;
	ActiveTint = StableTint;
	CurrentClosedness = StableClosedness;
	CurrentOpacity = StableOpacity;
	ApplyVisualSample(CurrentClosedness, CurrentOpacity);
	SetVisibility(ESlateVisibility::Collapsed);
}

void ULRStateEyeOverlayWidget::SetHUDWidgetController(ULRHUDWidgetController* controller)
{
	UnbindController();
	Super::SetHUDWidgetController(controller);

	if (HUDWidgetController)
	{
		HUDWidgetController->OnHoldStarted.AddDynamic(this, &ULRStateEyeOverlayWidget::HandleHoldStarted);
		HUDWidgetController->OnHoldCanceled.AddDynamic(this, &ULRStateEyeOverlayWidget::HandleHoldCanceled);
		HUDWidgetController->OnHoldThresholdReached.AddDynamic(
			this, &ULRStateEyeOverlayWidget::HandleHoldThresholdReached);
		HUDWidgetController->OnStateChangeRejected.AddDynamic(
			this, &ULRStateEyeOverlayWidget::HandleStateChangeRejected);
		HUDWidgetController->OnPerceptionModeChanged.AddDynamic(this, &ULRStateEyeOverlayWidget::HandleModeChanged);
		HandleModeChanged(HUDWidgetController->GetCurrentMode(), FGameplayTag());
	}
}

void ULRStateEyeOverlayWidget::SetScreenVisible(const bool bVisible)
{
	bScreenLayerVisible = bVisible;
	Super::SetScreenVisible(bVisible);
	if (!bVisible)
	{
		AnimationPhase = ELREyeOverlayAnimationPhase::Idle;
		ActiveInputType = ELRStateRequestType::None;
		CurrentClosedness = StableClosedness;
		CurrentOpacity = StableOpacity;
		ApplyVisualSample(CurrentClosedness, CurrentOpacity);
		SetVisibility(ESlateVisibility::Collapsed);
	}
	else if (AnimationPhase == ELREyeOverlayAnimationPhase::Idle)
	{
		// The state layer is persistent, but it should not tick/render while stable.
		SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ULRStateEyeOverlayWidget::NativeTick(const FGeometry& geometry, const float deltaTime)
{
	Super::NativeTick(geometry, deltaTime);
	if (AnimationPhase == ELREyeOverlayAnimationPhase::Idle)
	{
		return;
	}

	AnimationElapsed += FMath::Max(0.0f, deltaTime);
	const float alpha = AnimationDuration > MinInterpolationSeconds
		? FMath::Clamp(AnimationElapsed / AnimationDuration, 0.0f, 1.0f) : 1.0f;
	const float easedAlpha = EaseInOut(alpha);
	CurrentClosedness = FMath::Lerp(AnimationStartClosedness, AnimationTargetClosedness, easedAlpha);
	CurrentOpacity = FMath::Lerp(AnimationStartOpacity, AnimationTargetOpacity, easedAlpha);
	ApplyVisualSample(CurrentClosedness, CurrentOpacity);

	if (alpha >= 1.0f)
	{
		FinishInterpolation();
	}
}

void ULRStateEyeOverlayWidget::NativeDestruct()
{
	UnbindController();
	AnimationPhase = ELREyeOverlayAnimationPhase::Idle;
	ActiveInputType = ELRStateRequestType::None;
	Super::NativeDestruct();
}

void ULRStateEyeOverlayWidget::HandleHoldStarted(const ELRStateRequestType inputType,
	const ELRPerceptionMode targetMode, const float holdSeconds)
{
	if (!bScreenLayerVisible || (inputType != ELRStateRequestType::CloseEyes
		&& inputType != ELRStateRequestType::OpenEyes))
	{
		return;
	}

	ActiveInputType = inputType;
	const bool bOpening = inputType == ELRStateRequestType::OpenEyes;
	ActiveTint = HUDWidgetController
		? HUDWidgetController->GetEyeOverlayTint(targetMode) : FLinearColor::Black;
	const float activeOpacity = HUDWidgetController
		? HUDWidgetController->GetEyeOverlayMaxOpacity(targetMode) : HoldBlockOpacity;
	const float startClosedness = bOpening ? FullyClosedClosedness : FullyOpenClosedness;
	const float startOpacity = activeOpacity;
	const float openProgress = HUDWidgetController
		? HUDWidgetController->GetEyeOpenThresholdVisualProgress() : 0.90f;
	const float targetClosedness = bOpening ? 1.0f - openProgress : FullyClosedClosedness;
	const float targetOpacity = activeOpacity;
	StartInterpolation(startClosedness, targetClosedness, startOpacity, targetOpacity,
		FMath::Max(0.0f, holdSeconds), ELREyeOverlayAnimationPhase::Hold);
}

void ULRStateEyeOverlayWidget::HandleHoldCanceled(const ELRStateRequestType inputType)
{
	if (AnimationPhase != ELREyeOverlayAnimationPhase::Hold || ActiveInputType != inputType)
	{
		return;
	}
	RollbackToStable();
}

void ULRStateEyeOverlayWidget::HandleHoldThresholdReached(const ELRStateRequestType inputType,
	const ELRPerceptionMode targetMode)
{
	if (AnimationPhase != ELREyeOverlayAnimationPhase::Hold || ActiveInputType != inputType)
	{
		return;
	}

	const ULRHUDWidgetController* controller = HUDWidgetController;
	const bool bOpening = inputType == ELRStateRequestType::OpenEyes;
	const float openProgress = controller ? controller->GetEyeOpenThresholdVisualProgress() : 0.90f;
	const float thresholdClosedness = bOpening ? 1.0f - openProgress : FullyClosedClosedness;
	const float duration = controller ? controller->GetEyeOverlaySuccessTailSeconds() : 0.12f;
	// Snap to the exact gameplay threshold before the UI-only success tail, so a timer/tick
	// scheduling difference cannot leave the lids short of the transition boundary.
	CurrentClosedness = thresholdClosedness;
	CurrentOpacity = controller ? controller->GetEyeOverlayMaxOpacity(targetMode) : HoldBlockOpacity;
	ActiveTint = controller ? controller->GetEyeOverlayTint(targetMode) : ActiveTint;
	ApplyVisualSample(CurrentClosedness, CurrentOpacity);
	const float successTargetClosedness = bOpening ? FullyOpenClosedness : FullyClosedClosedness;
	StartInterpolation(CurrentClosedness, successTargetClosedness, CurrentOpacity, 0.0f, duration,
		ELREyeOverlayAnimationPhase::SuccessTail);
}

void ULRStateEyeOverlayWidget::HandleStateChangeRejected(const FLRStateChangeRequest request,
	const FGameplayTag reason)
{
	if ((AnimationPhase != ELREyeOverlayAnimationPhase::Hold
		&& AnimationPhase != ELREyeOverlayAnimationPhase::SuccessTail)
		|| ActiveInputType != request.RequestType)
	{
		return;
	}
	RollbackToStable();
	(void)reason;
}

void ULRStateEyeOverlayWidget::HandleModeChanged(const ELRPerceptionMode currentMode, const FGameplayTag reason)
{
	StableClosedness = currentMode == ELRPerceptionMode::Perception ? FullyClosedClosedness : FullyOpenClosedness;
	StableTint = HUDWidgetController
		? HUDWidgetController->GetEyeOverlayTint(currentMode) : FLinearColor::Black;
	// Stable state is represented by the world presentation. The lid blocks are transient and
	// must remain transparent/collapsed even while Perception keeps a closed geometry endpoint.
	StableOpacity = 0.0f;
	if (AnimationPhase == ELREyeOverlayAnimationPhase::Idle)
	{
		ActiveTint = StableTint;
		CurrentClosedness = StableClosedness;
		CurrentOpacity = StableOpacity;
		ApplyVisualSample(CurrentClosedness, CurrentOpacity);
		SetOverlayVisualVisible(false);
	}
	(void)reason;
}

void ULRStateEyeOverlayWidget::UnbindController()
{
	if (!HUDWidgetController)
	{
		return;
	}
	HUDWidgetController->OnHoldStarted.RemoveDynamic(this, &ULRStateEyeOverlayWidget::HandleHoldStarted);
	HUDWidgetController->OnHoldCanceled.RemoveDynamic(this, &ULRStateEyeOverlayWidget::HandleHoldCanceled);
	HUDWidgetController->OnHoldThresholdReached.RemoveDynamic(
		this, &ULRStateEyeOverlayWidget::HandleHoldThresholdReached);
	HUDWidgetController->OnStateChangeRejected.RemoveDynamic(
		this, &ULRStateEyeOverlayWidget::HandleStateChangeRejected);
	HUDWidgetController->OnPerceptionModeChanged.RemoveDynamic(this, &ULRStateEyeOverlayWidget::HandleModeChanged);
}

void ULRStateEyeOverlayWidget::ApplyVisualSample(const float closedness, const float opacity)
{
	if (!TopBlock || !BottomBlock)
	{
		return;
	}

	const float clampedClosedness = FMath::Clamp(closedness, 0.0f, 1.0f);
	const float clampedOpacity = FMath::Clamp(opacity, 0.0f, 1.0f);
	const float halfHeight = GetCachedGeometry().GetLocalSize().Y * 0.5f;
	const float openAmount = 1.0f - clampedClosedness;
	const FVector2D translation(0.0f, halfHeight * openAmount);
	TopBlock->SetRenderTranslation(FVector2D(0.0f, -translation.Y));
	BottomBlock->SetRenderTranslation(translation);
	TopBlock->SetColorAndOpacity(ActiveTint);
	BottomBlock->SetColorAndOpacity(ActiveTint);
	TopBlock->SetRenderOpacity(clampedOpacity);
	BottomBlock->SetRenderOpacity(clampedOpacity);
}

void ULRStateEyeOverlayWidget::SetOverlayVisualVisible(const bool bVisible)
{
	SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void ULRStateEyeOverlayWidget::StartInterpolation(const float startClosedness, const float targetClosedness,
	const float startOpacity, const float targetOpacity, const float duration,
	const ELREyeOverlayAnimationPhase phase)
{
	AnimationPhase = phase;
	AnimationElapsed = 0.0f;
	AnimationDuration = FMath::Max(0.0f, duration);
	AnimationStartClosedness = startClosedness;
	AnimationTargetClosedness = targetClosedness;
	AnimationStartOpacity = startOpacity;
	AnimationTargetOpacity = targetOpacity;
	CurrentClosedness = startClosedness;
	CurrentOpacity = startOpacity;
	SetOverlayVisualVisible(true);
	ApplyVisualSample(CurrentClosedness, CurrentOpacity);
}

void ULRStateEyeOverlayWidget::RollbackToStable()
{
	const ULRHUDWidgetController* controller = HUDWidgetController;
	const float duration = controller ? controller->GetEyeOverlayCancelSeconds() : 0.15f;
	StartInterpolation(CurrentClosedness, StableClosedness, CurrentOpacity, StableOpacity, duration,
		ELREyeOverlayAnimationPhase::Rollback);
}

void ULRStateEyeOverlayWidget::FinishInterpolation()
{
	CurrentClosedness = AnimationTargetClosedness;
	CurrentOpacity = AnimationTargetOpacity;
	if (AnimationPhase == ELREyeOverlayAnimationPhase::Rollback)
	{
		ActiveTint = StableTint;
	}
	if (AnimationPhase == ELREyeOverlayAnimationPhase::SuccessTail)
	{
		CurrentClosedness = AnimationTargetClosedness;
		CurrentOpacity = 0.0f;
	}
	else if (AnimationPhase == ELREyeOverlayAnimationPhase::Rollback)
	{
		CurrentClosedness = StableClosedness;
		CurrentOpacity = 0.0f;
	}
	ApplyVisualSample(CurrentClosedness, CurrentOpacity);
	AnimationPhase = ELREyeOverlayAnimationPhase::Idle;
	ActiveInputType = ELRStateRequestType::None;
	SetOverlayVisualVisible(CurrentOpacity > KINDA_SMALL_NUMBER);
}
