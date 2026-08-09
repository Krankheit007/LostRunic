#pragma once

#include "AI/LRGuardTypes.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"

#include "LRAlertComponent.generated.h"

class ULRGuardTuning;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FLRAlertChanged, int32, previousLevel, int32, currentLevel,
	ELRGuardBehaviorState, currentState, FGameplayTag, reason, FVector, disturbanceLocation);

/** Owns the guard's clamped 0-11 alert state and observation/decay timers. */
UCLASS(ClassGroup = "Lost Runic", BlueprintType, meta = (BlueprintSpawnableComponent, DisplayName = "Lost Runic Alert"))
class LOSTRUNIC_API ULRAlertComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULRAlertComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|AI|Alert")
	void ApplyAlertDelta(int32 delta, FVector location, AActor* target, FGameplayTag reason);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|AI|Alert")
	void SetSightTarget(AActor* target, bool bVisible, FVector lastKnownLocation);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|AI|Alert")
	void MarkInvestigationReached();

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|AI|Alert")
	void ResetAfterSearch();

	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Alert")
	int32 GetAlertLevel() const { return AlertLevel; }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Alert")
	ELRGuardBehaviorState GetBehaviorState() const;

	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Alert")
	FVector GetLastDisturbanceLocation() const { return LastDisturbanceLocation; }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Alert")
	AActor* GetTargetActor() const { return TargetActor.Get(); }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Alert")
	FGameplayTag GetLastReason() const { return LastReason; }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Alert")
	bool HasConfirmedSight() const { return bHasConfirmedSight; }

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|AI|Alert")
	FLRAlertChanged OnAlertChanged;

private:
	void HandleDecayTimer();
	void BroadcastChange(int32 previousLevel, ELRGuardBehaviorState previousState, FGameplayTag reason);
	const ULRGuardTuning& GetEffectiveTuning() const;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Alert", meta = (AllowPrivateAccess = "true"))
	int32 AlertLevel = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Alert", meta = (AllowPrivateAccess = "true"))
	FVector LastDisturbanceLocation = FVector::ZeroVector;

	UPROPERTY(Transient)
	TObjectPtr<ULRGuardTuning> Tuning;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> TargetActor;

	FGameplayTag LastReason;
	double LastStimulusTimeSeconds = 0.0;
	bool bHasConfirmedSight = false;
	bool bSearching = false;
	FTimerHandle DecayTimer;
};
