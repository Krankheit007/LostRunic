/**
 * @file LRAlertComponent.h
 * @brief Owns the guard's 0-11 alert meter, observation/decay timing and search flag.
 */
#pragma once

#include "AI/LRGuardTypes.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"

#include "LRAlertComponent.generated.h"

class ALRGuardAIController;
class ULRGuardTuning;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FLRAlertChanged, int32, previousLevel, int32, currentLevel,
	ELRGuardBehaviorState, currentState, FGameplayTag, reason, FVector, disturbanceLocation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRAlertSnapshotChanged, const FLRAlertSnapshot&, snapshot);
DECLARE_MULTICAST_DELEGATE(FLRAlertDecayRequested);

/** Alert meter state. Threat identity and evidence live in ULRGuardKnowledgeComponent. */
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
	bool IsSearching() const { return bSearching; }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Alert")
	bool IsObserving() const { return bObserving; }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Alert")
	FGameplayTag GetLastReason() const { return LastReason; }

	/** Pure alert presentation data. Behavior is filled only by the controller's awareness snapshot. */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Alert")
	FLRAlertSnapshot GetAlertSnapshot() const;

	/** Compatibility/diagnostic delegates; controller publishes after an awareness transaction commits. */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|AI|Alert")
	FLRAlertChanged OnAlertChanged;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|AI|Alert")
	FLRAlertSnapshotChanged OnAlertSnapshotChanged;

private:
	friend class ALRGuardAIController;

	bool ApplyDelta(int32 delta);
	bool RaiseToMinimum(int32 minimumLevel);
	bool LowerToMaximum(int32 maximumLevel);
	bool TryApplyAttract(double nowSeconds);
	void MarkInvestigationReached();
	void MarkInvestigationUnreachable();
	void ResetAfterSearch();
	void PublishCommittedChange(int32 previousLevel, ELRGuardBehaviorState resolvedBehavior,
		FGameplayTag reason, const FVector& location);
	void HandleDecayTimer();
	void HandleObservationEnd();
	void StartObservation();
	void ClearWhenAlertZero();
	const ULRGuardTuning& GetEffectiveTuning() const;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Alert", meta = (AllowPrivateAccess = "true"))
	int32 AlertLevel = 0;

	UPROPERTY(Transient)
	TObjectPtr<ULRGuardTuning> Tuning;

	FGameplayTag LastReason;
	double LastIncreaseTimeSeconds = 0.0;
	bool bSearching = false;
	bool bObserving = false;
	bool bFirstIncreaseInBand = false;
	FTimerHandle DecayTimer;
	FTimerHandle ObservationTimer;
	FLRAlertDecayRequested OnDecayRequested;
};
