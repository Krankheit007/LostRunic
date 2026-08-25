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

enum class ELRGuardBehaviorEntryResult : uint8
{
	Running,
	AlreadyAtGoal,
	Failed
};

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

	/** Number of actual investigation MoveTo requests issued since the current possession. */
	int32 GetInvestigationMoveRequestCount() const { return InvestigationMoveRequestCount; }

	/** Unified accepted/rejected noise domain entry for hearing and room propagation. */
	void ReceiveNoiseStimulus(const FLRGuardNoiseStimulus& stimulus);
	void MarkInvestigationReached();
	void MarkInvestigationUnreachable();
	void ResetSearch();
	bool IsRelevantSightTarget(const AActor* actor) const;

	ELRGuardBehaviorEntryResult EnterBehavior(ELRGuardBehaviorState behavior);
	void FinalizeStateTreeBehaviorEntry(ELRGuardBehaviorState behavior, ELRGuardBehaviorEntryResult result);
	void ExitBehavior(ELRGuardBehaviorState behavior);
	void LogAndDrawDiagnostics() const;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|AI|Awareness")
	FLRGuardAwarenessChanged OnGuardAwarenessChanged;

protected:
	virtual void OnMoveCompleted(FAIRequestID requestId, const FPathFollowingResult& result) override;

private:
#if WITH_DEV_AUTOMATION_TESTS
	friend class FLRGuardSightContactLifecycleRuntimeTest;
#endif
	UFUNCTION()
	void HandlePerception(AActor* actor, FAIStimulus stimulus);

	UFUNCTION()
	void HandleKnockback(FVector direction);

	void ConfigurePerception();
	void HandleCaptureCheck();
	void HandleDetectionSample();
	void HandleAlertDecayRequested();
	void HandleStunEnd();
	void StartPatrolMove();
	FLRGuardAwarenessSnapshot BuildCurrentAwarenessSnapshot() const;
	void ProcessAwarenessTransaction(FGameplayTag reason, bool bForcePublish = false);
	void RefreshBehaviorContext(const FLRGuardAwarenessSnapshot& current);
	void CommitAwareness(FGameplayTag reason, bool bForcePublish = false);
	void DeferAwarenessCommit(FGameplayTag reason, bool bForcePublish);
	void ClearInvestigationMoveRequest();
	void ClearInvestigationRetrySuppression();
	bool ShouldRetryInvestigationAt(const FVector& location) const;
	void ApplyInvestigationReached();
	void ApplyInvestigationUnreachable();
	void StartDetectionSampling();
	void StopDetectionSampling();
	FPathFollowingRequestResult RequestInvestigationMove(const FVector& location);
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
	FTimerHandle DetectionSampleTimer;

	bool bHasInvestigationMoveTarget = false;
	FVector CurrentInvestigationMoveTarget = FVector::ZeroVector;
	bool bInvestigationRetrySuppressed = false;
	bool bHasUnreachableInvestigationLocation = false;
	FVector LastUnreachableInvestigationLocation = FVector::ZeroVector;
	FAIRequestID InvestigationMoveRequestId = FAIRequestID::InvalidRequest;
	int32 InvestigationMoveRequestCount = 0;
	bool bHasSuspiciousFocusLocation = false;
	FVector CurrentSuspiciousFocusLocation = FVector::ZeroVector;
	bool bAwarenessCommitDeferred = false;
	FGameplayTag DeferredAwarenessReason;
	bool bDeferredForcePublish = false;
	FTimerHandle StunTimer;
};
