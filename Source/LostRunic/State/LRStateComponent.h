#pragma once

#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "State/LRStateRules.h"

#include "LRStateComponent.generated.h"

class ULRStateTuning;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FLRStateChanging, ELRPerceptionMode, previousMode,
	ELRPerceptionMode, nextMode, FGameplayTag, reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLRStateChanged, ELRPerceptionMode, currentMode, FGameplayTag, reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLRStateChangeRejected, FLRStateChangeRequest, request, FGameplayTag, reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FLRStateHoldStarted, ELRStateRequestType, inputType,
	ELRPerceptionMode, targetMode, float, holdSeconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRStateHoldCanceled, ELRStateRequestType, inputType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLRStateHoldThresholdReached, ELRStateRequestType, inputType,
	ELRPerceptionMode, targetMode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRPresentationLockChanged, bool, bLocked);

/** Owns perception state validation, hold input transactions, and presentation locking. */
UCLASS(ClassGroup = "Lost Runic", BlueprintType, meta = (BlueprintSpawnableComponent, DisplayName = "Lost Runic State"))
class LOSTRUNIC_API ULRStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULRStateComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|State")
	FLRStateChangeResult RequestStateChange(const FLRStateChangeRequest& request);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|State|Input")
	void BeginEyeInput(ELRStateRequestType inputType);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|State|Input")
	void EndEyeInput(ELRStateRequestType inputType);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|State|Input")
	void CancelEyeInputSequence();

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|State")
	void SetBlockerActive(FGameplayTag blocker, bool bActive);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|State|Presentation")
	void NotifyPresentationComplete();

	UFUNCTION(BlueprintPure, Category = "Lost Runic|State")
	ELRPerceptionMode GetCurrentMode() const { return CurrentMode; }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|State")
	bool IsPresentationLocked() const { return bPresentationLocked; }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|State")
	FGameplayTag GetLastTransitionReason() const { return LastTransitionReason; }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|State")
	FGameplayTagContainer GetActiveBlockers() const { return ActiveBlockers; }

	void LogDiagnostics() const;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|State")
	FLRStateChanging OnStateChanging;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|State")
	FLRStateChanged OnStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|State")
	FLRStateChangeRejected OnStateChangeRejected;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|State|Input")
	FLRStateHoldStarted OnHoldStarted;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|State|Input")
	FLRStateHoldCanceled OnHoldCanceled;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|State|Input")
	FLRStateHoldThresholdReached OnHoldThresholdReached;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|State|Presentation")
	FLRPresentationLockChanged OnPresentationLockChanged;

private:
	void HandleHoldThreshold();
	void HandlePresentationSafetyTimeout();
	void RejectRequest(const FLRStateChangeRequest& request, FGameplayTag reason);
	void StartPresentationLock();
	const ULRStateTuning& GetEffectiveTuning() const;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	ELRPerceptionMode CurrentMode = ELRPerceptionMode::Normal;

	UPROPERTY(Transient)
	TObjectPtr<ULRStateTuning> Tuning;

	UPROPERTY(Transient)
	FGameplayTagContainer ActiveBlockers;

	FGameplayTag LastTransitionReason;
	FLRStateInputGate InputGate;
	ELRPerceptionMode PendingInputTarget = ELRPerceptionMode::Normal;
	bool bPresentationLocked = false;
	FTimerHandle HoldTimer;
	FTimerHandle PresentationSafetyTimer;
};
