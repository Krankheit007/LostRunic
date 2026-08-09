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
class ULRGuardTuning;
class UStateTreeAIComponent;

/** Event-driven guard perception, navigation, StateTree, and capture coordinator. */
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

	void EnterBehavior(ELRGuardBehaviorState behavior);
	void ExitBehavior(ELRGuardBehaviorState behavior);
	void LogAndDrawDiagnostics() const;

protected:
	virtual void OnMoveCompleted(FAIRequestID requestId, const FPathFollowingResult& result) override;

private:
	UFUNCTION()
	void HandlePerception(AActor* actor, FAIStimulus stimulus);

	UFUNCTION()
	void HandleAlertChanged(int32 previousLevel, int32 currentLevel, ELRGuardBehaviorState currentState,
		FGameplayTag reason, FVector disturbanceLocation);

	void ConfigurePerception();
	void HandleCaptureTimer();
	void HandleSearchTimeout();
	void StartPatrolMove();
	bool CanConfirmSight(AActor* actor) const;
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
	TWeakObjectPtr<ULRAlertComponent> Alert;

	ELRGuardBehaviorState ActiveBehavior = ELRGuardBehaviorState::IdlePatrol;
	int32 PatrolIndex = 0;
	FTimerHandle CaptureTimer;
	FTimerHandle SearchTimer;
};
