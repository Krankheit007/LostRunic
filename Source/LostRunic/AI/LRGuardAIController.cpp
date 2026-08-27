/**
 * @file LRGuardAIController.cpp
 * @brief Guard controller lifecycle and transactional Awareness coordination.
 */
#include "AI/LRGuardAIController.h"

#include "AI/LRAlertComponent.h"
#include "AI/LRAlertRules.h"
#include "AI/LRGuardCharacter.h"
#include "AI/LRGuardKnowledgeComponent.h"
#include "AI/LRGuardPerceptionRules.h"
#include "Components/StateTreeAIComponent.h"
#include "Core/LRGameplayTags.h"
#include "Core/LRLog.h"
#include "Data/LRGameTuningSet.h"
#include "Data/LRStateTuning.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "Items/LRCourageResponseComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Navigation/PathFollowingComponent.h"
#include "TimerManager.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

ALRGuardAIController::ALRGuardAIController()
{
	PrimaryActorTick.bCanEverTick = false;
	// StateTree depends on guard-owned Alert/Knowledge context. Start it explicitly
	// at the end of OnPossess, after those dependencies have been resolved.
	bStartAILogicOnPossess = false;
	bStopAILogicOnUnposses = true;
	bAttachToPawn = true;
	StateTreeAI = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAI"));
	StateTreeAI->SetStartLogicAutomatically(false);
	AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
	AIPerception->SetAutoActivate(false);
	SetPerceptionComponent(*AIPerception);
}

void ALRGuardAIController::BeginPlay()
{
	Super::BeginPlay();
	const UGameInstance* gameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const ULRGameInstanceSubsystem* subsystem = gameInstance
		? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	if (subsystem && subsystem->GetTuningSet())
	{
		StateTuning = subsystem->GetTuningSet()->State;
	}
	LastDetectionSampleTime = 0.0;
	CachedAwareness = BuildCurrentAwarenessSnapshot();
	TryInitializeRuntime();
}
void ALRGuardAIController::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	ShutdownRuntime();
	Super::EndPlay(endPlayReason);
}
void ALRGuardAIController::OnPossess(APawn* inPawn)
{
	Super::OnPossess(inPawn);
	ALRGuardCharacter* guard = Cast<ALRGuardCharacter>(inPawn);
	Alert = guard ? guard->GetAlertComponent() : nullptr;
	Knowledge = guard ? guard->GetKnowledgeComponent() : nullptr;
	if (guard)
	{
		if (ULRCourageResponseComponent* courage = guard->GetCourageResponseComponent())
		{
			courage->OnKnockbackApplied.AddUniqueDynamic(this, &ALRGuardAIController::HandleKnockback);
		}
	}
	CachedAwareness = BuildCurrentAwarenessSnapshot();
	InvestigationMoveRequestCount = 0;
	ClearInvestigationRetrySuppression();
	LastDetectionSampleTime = 0.0;
	TryInitializeRuntime();
}

bool ALRGuardAIController::ValidateControllerConfiguration(FString& outError,
	const bool bRequirePossessionContext) const
{
	if (!Tuning.Validate(outError))
	{
		outError = FString::Printf(TEXT("Guard tuning is invalid: %s"), *outError);
		return false;
	}
	if (!AIPerception || !StateTreeAI)
	{
		outError = TEXT("Native AIPerception and StateTreeAI components are required.");
		return false;
	}
	if (!AIPerception->GetSenseConfig<UAISenseConfig_Sight>()
		|| !AIPerception->GetSenseConfig<UAISenseConfig_Hearing>())
	{
		outError = TEXT("Inherited AIPerception must configure both Sight and Hearing.");
		return false;
	}
	if (AIPerception->GetDominantSense() != UAISense_Sight::StaticClass())
	{
		outError = FString::Printf(TEXT("Sight must be the Guard dominant sense; configured=%s."),
			*GetNameSafe(AIPerception->GetDominantSense()));
		return false;
	}
	if (bRequirePossessionContext
		&& (!Cast<ALRGuardCharacter>(GetPawn()) || !Alert.IsValid() || !Knowledge.IsValid() || !StateTuning))
	{
		outError = TEXT("Possessed Guard, Alert, Knowledge, and State tuning are required.");
		return false;
	}
	return true;
}

void ALRGuardAIController::TryInitializeRuntime()
{
	if (bRuntimeInitialized || !HasActorBegunPlay() || !GetPawn())
	{
		return;
	}

	FString error;
	if (!ValidateControllerConfiguration(error, true))
	{
		UE_LOG(LogLostRunicAI, Error, TEXT("Controller=%s Pawn=%s runtime initialization rejected: %s"),
			*GetNameSafe(this), *GetNameSafe(GetPawn()), *error);
		return;
	}

	Alert->InitializeRuntime(Tuning);
	Alert->OnDecayRequested.RemoveAll(this);
	Alert->OnDecayRequested.AddUObject(this, &ALRGuardAIController::HandleAlertDecayRequested);
	AIPerception->OnTargetPerceptionUpdated.RemoveDynamic(this, &ALRGuardAIController::HandlePerception);
	AIPerception->OnTargetPerceptionUpdated.AddDynamic(this, &ALRGuardAIController::HandlePerception);
	AIPerception->Activate(true);
	if (!AIPerception->IsActive())
	{
		UE_LOG(LogLostRunicAI, Error, TEXT("Controller=%s Pawn=%s failed to activate AIPerception."),
			*GetNameSafe(this), *GetNameSafe(GetPawn()));
		ShutdownRuntime();
		return;
	}

	StateTreeAI->StartLogic();
	if (!StateTreeAI->IsRunning())
	{
		UE_LOG(LogLostRunicAI, Error, TEXT("Controller=%s Pawn=%s failed to start configured StateTree."),
			*GetNameSafe(this), *GetNameSafe(GetPawn()));
		ShutdownRuntime();
		return;
	}
	bRuntimeInitialized = true;
}

void ALRGuardAIController::ShutdownRuntime()
{
	if (AIPerception)
	{
		AIPerception->OnTargetPerceptionUpdated.RemoveDynamic(this, &ALRGuardAIController::HandlePerception);
		AIPerception->ForgetAll();
		AIPerception->Deactivate();
	}
	if (StateTreeAI && StateTreeAI->IsRunning())
	{
		StateTreeAI->StopLogic(TEXT("Guard runtime shutdown"));
	}
	StopDetectionSampling();
	StopMovement();
	ClearFocus(EAIFocusPriority::Gameplay);
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(StunTimer);
	}
	if (Alert.IsValid())
	{
		Alert->OnDecayRequested.RemoveAll(this);
		Alert->ShutdownRuntime();
	}
	bRuntimeInitialized = false;
}
void ALRGuardAIController::OnUnPossess()
{
	ShutdownRuntime();
	if (ALRGuardCharacter* guard = Cast<ALRGuardCharacter>(GetPawn()))
	{
		if (ULRCourageResponseComponent* courage = guard->GetCourageResponseComponent())
		{
			courage->OnKnockbackApplied.RemoveDynamic(this, &ALRGuardAIController::HandleKnockback);
		}
		if (Knowledge.IsValid())
		{
			Knowledge->SuspendVisualContact();
		}
	}
	PerceivedSightContact.Reset();
	LastDetectionSampleTime = 0.0;
	ClearInvestigationMoveRequest();
	ClearInvestigationRetrySuppression();
	bAwarenessCommitDeferred = false;
	DeferredAwarenessReason = FGameplayTag();
	bDeferredForcePublish = false;
	bHasSuspiciousFocusLocation = false;
	Alert.Reset();
	Knowledge.Reset();
	CachedAwareness = FLRGuardAwarenessSnapshot();
	Super::OnUnPossess();
}

#if WITH_EDITOR
EDataValidationResult ALRGuardAIController::IsDataValid(FDataValidationContext& context) const
{
	FString error;
	if (!ValidateControllerConfiguration(error, false))
	{
		context.AddError(FText::FromString(error));
		return EDataValidationResult::Invalid;
	}
	return Super::IsDataValid(context);
}
#endif
FLRGuardAwarenessSnapshot ALRGuardAIController::BuildCurrentAwarenessSnapshot() const
{
	FLRGuardAwarenessSnapshot snapshot;
	if (Alert.IsValid())
	{
		snapshot.Alert = Alert->GetAlertSnapshot();
	}
	if (Knowledge.IsValid())
	{
		snapshot.Knowledge = Knowledge->GetSnapshot();
	}
	snapshot.ResolvedBehavior = LRAlertRules::ResolveTargetBehavior(snapshot.Alert, snapshot.Knowledge,
		bStunned, Alert.IsValid() && Alert->IsSearching(), GetEffectiveTuning());
	snapshot.InvestigationLocation = LRAlertRules::ResolveInvestigationLocation(snapshot.Knowledge);
	snapshot.Alert.Behavior = snapshot.ResolvedBehavior;
	return snapshot;
}

FLRGuardAwarenessSnapshot ALRGuardAIController::GetAwarenessSnapshot() const
{
	return CachedAwareness;
}

ELRGuardBehaviorState ALRGuardAIController::GetResolvedBehavior() const
{
	return BuildCurrentAwarenessSnapshot().ResolvedBehavior;
}

void ALRGuardAIController::ReceiveNoiseStimulus(const FLRGuardNoiseStimulus& stimulus)
{
	if (!Alert.IsValid() || !Knowledge.IsValid() || !stimulus.Reason.IsValid())
	{
		return;
	}
	const FLRGuardAwarenessSnapshot previous = BuildCurrentAwarenessSnapshot();
	const int32 previousLevel = Alert->GetAlertLevel();
	FLRNoiseResponse response = LRGuardPerceptionRules::ResolveNoiseAlertDelta(
		stimulus.Reason, previousLevel, GetEffectiveTuning());
	if (stimulus.PropagationMode == ELRGuardNoisePropagationMode::CurrentRoom)
	{
		response.bRespond = true;
		response.bIsAttract = false;
		response.Delta = FMath::Max(GetEffectiveTuning().RoomRunAlertLevel - previousLevel, 0);
	}
	else if (stimulus.PropagationMode == ELRGuardNoisePropagationMode::AdjacentRoom)
	{
		response.bRespond = true;
		response.bIsAttract = false;
		response.Delta = GetEffectiveTuning().AdjacentRoomRunAlertAmount;
	}
	if (!response.bRespond)
	{
		return;
	}

	const double now = GetWorld() ? GetWorld()->GetTimeSeconds() : stimulus.TimeSeconds;
	if (response.bIsAttract)
	{
		if (!Alert->TryApplyAttract(now))
		{
			return;
		}
	}
	else
	{
		Alert->ApplyDelta(response.Delta);
	}
	const AActor* confirmed = previous.Knowledge.ConfirmedThreat.Get();
	Knowledge->CommitAcceptedNoise(stimulus, confirmed && confirmed == stimulus.Source.Get());
	ProcessAwarenessTransaction(stimulus.Reason);
}
void ALRGuardAIController::MarkInvestigationReached()
{
	if (!Alert.IsValid() || !Knowledge.IsValid())
	{
		return;
	}
	ApplyInvestigationReached();
	ProcessAwarenessTransaction(LRGameplayTags::SearchReached, true);
}

void ALRGuardAIController::MarkInvestigationUnreachable()
{
	if (!Alert.IsValid() || !Knowledge.IsValid())
	{
		return;
	}
	ApplyInvestigationUnreachable();
	ProcessAwarenessTransaction(LRGameplayTags::SearchUnreachable, true);
}
void ALRGuardAIController::ResetSearch()
{
	if (!Alert.IsValid() || !Knowledge.IsValid())
	{
		return;
	}
	Alert->ResetAfterSearch();
	Knowledge->ResetAwareness();
	PerceivedSightContact.Reset();
	StopDetectionSampling();
	LastDetectionSampleTime = 0.0;
	ClearInvestigationMoveRequest();
	ClearInvestigationRetrySuppression();
	ProcessAwarenessTransaction(LRGameplayTags::SearchTimeout, true);
}
void ALRGuardAIController::HandleAlertDecayRequested()
{
	if (!Alert.IsValid() || !Knowledge.IsValid())
	{
		return;
	}
	const FLRGuardAwarenessSnapshot previous = BuildCurrentAwarenessSnapshot();
	if (!LRAlertRules::ShouldDecay(previous.Alert, previous.Knowledge, Alert->IsObserving(),
		previous.ResolvedBehavior))
	{
		return;
	}
	Alert->ApplyDelta(-GetEffectiveTuning().AlertDecayAmount);
	if (Alert->GetAlertLevel() == 0)
	{
		Knowledge->ResetAwareness();
		PerceivedSightContact.Reset();
		StopDetectionSampling();
		LastDetectionSampleTime = 0.0;
		ClearInvestigationMoveRequest();
		ClearInvestigationRetrySuppression();
	}
	ProcessAwarenessTransaction(LRGameplayTags::SearchAlertDecay);
}
const FLRGuardTuningSettings& ALRGuardAIController::GetEffectiveTuning() const
{
	return Tuning;
}
