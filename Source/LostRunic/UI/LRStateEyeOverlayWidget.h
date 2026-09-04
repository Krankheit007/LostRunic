/**
 * @file LRStateEyeOverlayWidget.h
 * @brief Presents the eye-hold close/open transition without owning state timing or gameplay completion.
 */
#pragma once

#include "Layout/Geometry.h"
#include "State/LRStateTypes.h"
#include "UI/LRScreenWidget.h"

#include "LRStateEyeOverlayWidget.generated.h"

class UImage;
class ULRHUDWidgetController;

/** Internal visual phase; state authority remains in ULRStateComponent. */
enum class ELREyeOverlayAnimationPhase : uint8
{
	Idle,
	Hold,
	SuccessTail,
	Rollback
};

/**
 * @brief Full-screen eye-lid overlay driven only by HUD controller events.
 *
 * The widget never queries StateComponent, world objects, timers, or completion state. It only
 * interpolates its two bound blocks while a controller event is active. State presentation
 * completion remains owned by ULRStatePresentationComponent/PresentStateChange.
 */
UCLASS(Blueprintable, meta = (DisplayName = "Lost Runic State Eye Overlay Widget"))
class LOSTRUNIC_API ULRStateEyeOverlayWidget : public ULRScreenWidget
{
	GENERATED_BODY()

public:
	/** Injects the UI event source and binds the four state-input forwarding delegates. */
	virtual void SetHUDWidgetController(ULRHUDWidgetController* controller) override;
	/** Keeps the state-overlay screen available while collapsing it outside an active transition. */
	virtual void SetScreenVisible(bool bVisible) override;

protected:
	/** Initializes the two-block visual contract and pass-through input policy. */
	virtual void NativeOnInitialized() override;
	/** Advances only an active overlay interpolation; no gameplay state is sampled here. */
	virtual void NativeTick(const FGeometry& geometry, float deltaTime) override;
	/** Unbinds controller delegates before the widget is destroyed. */
	virtual void NativeDestruct() override;

	/** Explicit WBP_LRStateEyeOverlay contract: top block image. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> TopBlock;

	/** Explicit WBP_LRStateEyeOverlay contract: bottom block image. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> BottomBlock;

	/** Visual opacity of the two eye-lid blocks during hold/closed presentation. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Eye Overlay",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HoldBlockOpacity = 0.80f;

private:
	/** Forwards a state hold start into the close/open visual path. */
	UFUNCTION()
	void HandleHoldStarted(ELRStateRequestType inputType, ELRPerceptionMode targetMode, float holdSeconds);
	/** Reverses an uncommitted hold in the bounded cancellation window. */
	UFUNCTION()
	void HandleHoldCanceled(ELRStateRequestType inputType);
	/** Starts the success tail at the exact gameplay threshold. */
	UFUNCTION()
	void HandleHoldThresholdReached(ELRStateRequestType inputType, ELRPerceptionMode targetMode);
	/** Rolls back a rejected request without retrying or mutating gameplay state. */
	UFUNCTION()
	void HandleStateChangeRejected(FLRStateChangeRequest request, FGameplayTag reason);
	/** Tracks committed mode only through the controller's existing mode event. */
	UFUNCTION()
	void HandleModeChanged(ELRPerceptionMode currentMode, FGameplayTag reason);

	/** Removes all event bindings from the current controller. */
	void UnbindController();
	/** Applies a single visual sample to both blocks. */
	void ApplyVisualSample(float closedness, float opacity);
	/** Sets visibility without changing the controller-owned screen lifecycle flag. */
	void SetOverlayVisualVisible(bool bVisible);
	/** Starts an interpolation from the current sample to a target sample. */
	void StartInterpolation(float startClosedness, float targetClosedness, float startOpacity,
		float targetOpacity, float duration, ELREyeOverlayAnimationPhase phase);
	/** Restores the last committed visual state after a cancellation/rejection. */
	void RollbackToStable();
	/** Finishes an interpolation and collapses the overlay when it is fully transparent. */
	void FinishInterpolation();

	/** Current interpolation phase; idle widgets remain collapsed and do not render. */
	ELREyeOverlayAnimationPhase AnimationPhase = ELREyeOverlayAnimationPhase::Idle;
	ELRStateRequestType ActiveInputType = ELRStateRequestType::None;
	float AnimationElapsed = 0.0f;
	float AnimationDuration = 0.0f;
	float AnimationStartClosedness = 0.0f;
	float AnimationTargetClosedness = 0.0f;
	float AnimationStartOpacity = 0.0f;
	float AnimationTargetOpacity = 0.0f;
	float CurrentClosedness = 0.0f;
	float CurrentOpacity = 0.0f;
	float StableClosedness = 0.0f;
	float StableOpacity = 0.0f;
	FLinearColor StableTint = FLinearColor::Black;
	FLinearColor ActiveTint = FLinearColor::Black;
	bool bScreenLayerVisible = false;
};
