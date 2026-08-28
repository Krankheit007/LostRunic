/**
 * @file LRAlertComponent.h
 * @brief 拥有 Guard 0-11 警戒值和白色/红色观察、自然衰减、噪声冷却计时。
 */
#pragma once

#include "AI/LRGuardTypes.h"
#include "Components/ActorComponent.h"
#include "Data/LRGuardTuning.h"
#include "GameplayTagContainer.h"

#include "LRAlertComponent.generated.h"

class ALRGuardAIController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FLRAlertChanged, int32, previousLevel, int32, currentLevel,
	ELRGuardBehaviorState, currentState, FGameplayTag, reason, FVector, disturbanceLocation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRAlertSnapshotChanged, const FLRAlertSnapshot&, snapshot);
DECLARE_MULTICAST_DELEGATE(FLRAlertDecayRequested);

/** Alert 计时模式只描述警戒条为什么暂时不衰减，不代表 StateTree Gameplay 状态。 */
enum class ELRGuardAlertTimerMode : uint8
{
	None,
	WhiteObservation,
	RedObservation,
	Decay
};

/** Alert 数值和计时的唯一所有者；导航和 Investigate Moving 由 Controller 管理。 */
UCLASS(ClassGroup = "Lost Runic", BlueprintType, meta = (BlueprintSpawnableComponent, DisplayName = "Lost Runic Alert"))
class LOSTRUNIC_API ULRAlertComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULRAlertComponent();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Alert")
	int32 GetAlertLevel() const { return AlertLevel; }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Alert")
	bool IsObserving() const { return TimerMode == ELRGuardAlertTimerMode::WhiteObservation
		|| TimerMode == ELRGuardAlertTimerMode::RedObservation; }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Alert")
	bool IsDecaying() const { return TimerMode == ELRGuardAlertTimerMode::Decay; }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Alert")
	bool IsAttractCooldownActive() const;

	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Alert")
	FGameplayTag GetLastReason() const { return LastReason; }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Alert")
	FLRAlertSnapshot GetAlertSnapshot() const;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|AI|Alert")
	FLRAlertChanged OnAlertChanged;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|AI|Alert")
	FLRAlertSnapshotChanged OnAlertSnapshotChanged;

private:
	friend class ALRGuardAIController;
#if WITH_DEV_AUTOMATION_TESTS
	friend class FLRAlertSnapshotPresentationTest;
	friend class FLRGuardSightContactLifecycleRuntimeTest;
#endif

	bool ApplyDelta(int32 delta);
	void ApplySightAlertLevel();
	bool CanAcceptAttract(double nowSeconds) const;
	bool IsFirstAttractInResultBand(int32 resultAlertLevel) const;
	bool ApplyAcceptedAttract(int32 resultAlertLevel, double nowSeconds, float cooldownSeconds,
		bool bStartWhiteObservation);
	void StartWhiteObservation();
	void StartRedObservation();
	void StopObservationAndDecay();
	void ResetToZero();
	void PublishCommittedChange(int32 previousLevel, ELRGuardBehaviorState resolvedBehavior,
		FGameplayTag reason, const FVector& location);

	void HandleDecayTimer();
	void HandleObservationEnd();
	void HandleAttractCooldownEnd();
	void StartObservation(ELRGuardAlertTimerMode mode);
	void StartDecay();
	void StartAttractCooldown(double nowSeconds, float cooldownSeconds);
	void ClearWhenAlertZero();
	void InitializeRuntime(const FLRGuardTuningSettings& tuning);
	void ShutdownRuntime();
	const FLRGuardTuningSettings& GetEffectiveTuning() const;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Alert", meta = (AllowPrivateAccess = "true"))
	int32 AlertLevel = 0;

	FLRGuardTuningSettings RuntimeTuning;
	FGameplayTag LastReason;
	double AttractCooldownEndTimeSeconds = 0.0;
	double ObservationEndTimeSeconds = 0.0;
	bool bWhiteAttractAccepted = false;
	bool bRedAttractAccepted = false;
	bool bRuntimeInitialized = false;
	ELRGuardAlertTimerMode TimerMode = ELRGuardAlertTimerMode::None;

	FTimerHandle DecayTimer;
	FTimerHandle ObservationTimer;
	FTimerHandle AttractCooldownTimer;
	FLRAlertDecayRequested OnDecayRequested;
};
