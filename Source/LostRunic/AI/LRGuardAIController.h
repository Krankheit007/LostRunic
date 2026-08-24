/**
 * @file LRGuardAIController.h
 * @brief Adapts UE perception/world queries into transactional Guard Awareness and executes resolved StateTree behavior.
 */
#pragma once

#include "AIController.h"
#include "AI/LRGuardTypes.h"
#include "Perception/AIPerceptionTypes.h"

#include "LRGuardAIController.generated.h"

class ALRGuardCharacter;
class UAIPerceptionComponent;
class UAISenseConfig_Hearing;
class UAISenseConfig_Sight;
class ULRAlertComponent;
class ULRGuardKnowledgeComponent;
class ULRGuardTuning;
class ULRStateTuning;
class UStateTreeAIComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRGuardAwarenessChanged,
	const FLRGuardAwarenessSnapshot&, snapshot);

/** Controller-owned perception adapter and coordinator for sibling Alert/Knowledge components. */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Guard AI Controller"))
class LOSTRUNIC_API ALRGuardAIController : public AAIController
{
	GENERATED_BODY()

public:
	ALRGuardAIController();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
	virtual void OnPossess(APawn* inPawn) override;
	virtual void OnUnPossess() override;

	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI")
	ULRAlertComponent* GetAlertComponent() const { return Alert.Get(); }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI")
	ULRGuardKnowledgeComponent* GetKnowledgeComponent() const { return Knowledge.Get(); }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI")
	FLRGuardAwarenessSnapshot GetAwarenessSnapshot() const;

	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI")
	ELRGuardBehaviorState GetResolvedBehavior() const;

	/** Unified accepted/rejected noise domain entry for hearing and room propagation. */
	void ReceiveNoiseStimulus(const FLRGuardNoiseStimulus& stimulus);
	void MarkInvestigationReached();
	void ResetSearch();
	bool IsRelevantSightTarget(const AActor* actor) const;

	void EnterBehavior(ELRGuardBehaviorState behavior);
	void ExitBehavior(ELRGuardBehaviorState behavior);
	void LogAndDrawDiagnostics() const;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|AI|Awareness")
	FLRGuardAwarenessChanged OnGuardAwarenessChanged;

protected:
	virtual void OnMoveCompleted(FAIRequestID requestId, const FPathFollowingResult& result) override;

private:
	UFUNCTION()
	void HandlePerception(AActor* actor, FAIStimulus stimulus);

	UFUNCTION()
	void HandleKnockback(FVector direction);

	void ConfigurePerception();
	void HandleCaptureTimer();
	void HandleDetectionSample();
	void HandleAlertDecayRequested();
	void HandleStunEnd();
	void StartPatrolMove();
	void RefreshBehaviorContext(const FLRGuardAwarenessSnapshot& previous,
		const FLRGuardAwarenessSnapshot& current);
	void CommitAwareness(const FLRGuardAwarenessSnapshot& previous, int32 previousAlertLevel,
		FGameplayTag reason, bool bForcePublish = false);
	void HandleSightLost(AActor* actor, const FVector& lastKnownLocation);
	FLRGuardVisibilityResult EvaluateVisibility(AActor* actor) const;
	float ResolveMovementVisibilityFactor(const AActor* actor) const;
	bool IsHiddenFromGuard(AActor* actor) const;
	const ULRGuardTuning& GetEffectiveTuning() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStateTreeAIComponent> StateTreeAI;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAIPerceptionComponent> AIPerception;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	UPROPERTY(Transient)
	TObjectPtr<ULRGuardTuning> Tuning;

	UPROPERTY(Transient)
	TObjectPtr<ULRStateTuning> StateTuning;

	UPROPERTY(Transient)
	TWeakObjectPtr<ULRAlertComponent> Alert;

	UPROPERTY(Transient)
	TWeakObjectPtr<ULRGuardKnowledgeComponent> Knowledge;

	FLRGuardAwarenessSnapshot CachedAwareness;
	TWeakObjectPtr<AActor> PerceivedSightContact;
	double LastDetectionSampleTime = 0.0;
	ELRGuardBehaviorState ActiveBehavior = ELRGuardBehaviorState::IdlePatrol;
	int32 PatrolIndex = 0;
	bool bStunned = false;
	FTimerHandle CaptureTimer;
	FTimerHandle DetectionSampleTimer;
	FTimerHandle StunTimer;
};
