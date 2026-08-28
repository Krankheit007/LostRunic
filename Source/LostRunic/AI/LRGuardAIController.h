/**
 * @file LRGuardAIController.h
 * @brief 将 UE Perception、Alert、Knowledge、Navigation 和 Guard StateTree 接在一起。
 */
#pragma once

#include "AIController.h"
#include "AI/LRGuardTypes.h"
#include "Data/LRGuardTuning.h"
#include "Perception/AIPerceptionTypes.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include "LRGuardAIController.generated.h"

class ALRGuardCharacter;
class UAIPerceptionComponent;
class ULRAlertComponent;
class ULRGuardKnowledgeComponent;
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

enum class ELRGuardInvestigationMoveStatus : uint8
{
	None,
	Moving,
	AtLocation,
	Failed
};

/** Guard 感知入口和行为执行协调器。 */
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

	bool ValidateControllerConfiguration(FString& outError, bool bRequirePossessionContext) const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif

	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI")
	ULRAlertComponent* GetAlertComponent() const { return Alert.Get(); }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI")
	ULRGuardKnowledgeComponent* GetKnowledgeComponent() const { return Knowledge.Get(); }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI")
	FLRGuardAwarenessSnapshot GetAwarenessSnapshot() const;

	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI")
	ELRGuardBehaviorState GetResolvedBehavior() const;

	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Perception")
	bool HasRawSightContact() const { return bHasRawSightContact; }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Perception")
	bool IsSightToChaseGraceActive() const { return bSightToChaseGraceActive; }

	/** 当前持有的调查导航请求数量，用于诊断和回归测试。 */
	int32 GetInvestigationMoveRequestCount() const { return InvestigationMoveRequestCount; }

	/** UE Hearing、当前房和相邻房噪声统一进入这里。 */
	void ReceiveNoiseStimulus(const FLRGuardNoiseStimulus& stimulus);

	/** 导航完成/失败回调的行为入口；Grace 活跃时只记录结果，Grace 结束后抵达或失败均可开始红色观察。 */
	void MarkInvestigationReached();
	void MarkInvestigationUnreachable();

	bool IsRelevantSightTarget(const AActor* actor) const;

	ELRGuardBehaviorEntryResult EnterBehavior(ELRGuardBehaviorState behavior);
	void FinalizeStateTreeBehaviorEntry(ELRGuardBehaviorState behavior,
		ELRGuardBehaviorEntryResult result);
	void ExitBehavior(ELRGuardBehaviorState behavior);
	void LogAndDrawDiagnostics() const;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|AI|Awareness")
	FLRGuardAwarenessChanged OnGuardAwarenessChanged;

protected:
	virtual void OnMoveCompleted(FAIRequestID requestId,
		const FPathFollowingResult& result) override;

private:
#if WITH_DEV_AUTOMATION_TESTS
	friend class FLRGuardSightContactLifecycleRuntimeTest;
#endif

	UFUNCTION()
	void HandlePerception(AActor* actor, FAIStimulus stimulus);

	UFUNCTION()
	void HandleKnockback(FVector direction);

	void TryInitializeRuntime();
	void ShutdownRuntime();

	void HandleCaptureCheck();
	void HandleSightTracking();
	void HandleSightGraceExpired();
	void HandleAlertDecayRequested();
	void HandleStunEnd();
	void StartPatrolMove();

	void HandleSightAcquiredOrTracked(AActor* actor);
	void HandleSightLost(AActor* actor, const FVector& lastKnownLocation);
	void HandleAttractStimulus(const FLRGuardNoiseStimulus& stimulus);

	bool CanCurrentlySeeTarget() const;
	bool IsHiddenFromGuard(AActor* actor) const;
	AActor* GetActiveVisualCandidate() const;

	void StartSightTracking();
	void StopSightTracking();
	void StartSightToChaseGrace();
	void StopSightToChaseGrace();

	FLRGuardAwarenessSnapshot BuildCurrentAwarenessSnapshot() const;
	void ProcessAwarenessTransaction(FGameplayTag reason, bool bForcePublish = false);
	void RefreshBehaviorContext(const FLRGuardAwarenessSnapshot& current);
	void CommitAwareness(FGameplayTag reason, bool bForcePublish = false);
	void DeferAwarenessCommit(FGameplayTag reason, bool bForcePublish);

	void ClearInvestigationMoveRequest();
	void SetInvestigationAtLocation();
	void SetInvestigationFailed();
	void RequestInvestigationObservationIfReady();
	FPathFollowingRequestResult RequestInvestigationMove(const FVector& location,
		bool bForceRetarget = false);

	const FLRGuardTuningSettings& GetEffectiveTuning() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStateTreeAIComponent> StateTreeAI;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAIPerceptionComponent> AIPerception;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|调优",
		meta = (AllowPrivateAccess = "true"))
	FLRGuardTuningSettings Tuning;

	UPROPERTY(Transient)
	TObjectPtr<ULRStateTuning> StateTuning;

	UPROPERTY(Transient)
	TWeakObjectPtr<ULRAlertComponent> Alert;

	UPROPERTY(Transient)
	TWeakObjectPtr<ULRGuardKnowledgeComponent> Knowledge;

	FLRGuardAwarenessSnapshot CachedAwareness;
	TWeakObjectPtr<AActor> RawSightContact;
	ELRGuardBehaviorState ActiveBehavior = ELRGuardBehaviorState::IdlePatrol;

	int32 PatrolIndex = 0;
	int32 InvestigationMoveRequestCount = 0;
	FAIRequestID InvestigationMoveRequestId = FAIRequestID::InvalidRequest;
	FVector CurrentInvestigationMoveTarget = FVector::ZeroVector;
	ELRGuardInvestigationMoveStatus InvestigationMoveStatus = ELRGuardInvestigationMoveStatus::None;

	bool bStunned = false;
	bool bRuntimeInitialized = false;
	bool bHasRawSightContact = false;
	bool bSightToChaseGraceActive = false;
	bool bSightToChaseGraceConsumed = false;
	bool bForceInvestigationRetarget = false;
	bool bHasSuspiciousFocusLocation = false;
	bool bAwarenessCommitDeferred = false;
	bool bDeferredForcePublish = false;

	FTimerHandle SightTrackingTimer;
	FTimerHandle SightToChaseGraceTimer;
	FTimerHandle StunTimer;

	FGameplayTag DeferredAwarenessReason;
	FVector CurrentSuspiciousFocusLocation = FVector::ZeroVector;
};
