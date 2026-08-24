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
#include "Data/LRGuardDefinition.h"
#include "Data/LRGuardTuning.h"
#include "Data/LRStateTuning.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "Items/LRCourageResponseComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "TimerManager.h"

ALRGuardAIController::ALRGuardAIController()
{
	PrimaryActorTick.bCanEverTick = false;
	bStartAILogicOnPossess = true;
	bStopAILogicOnUnposses = true;
	bAttachToPawn = true;
	StateTreeAI = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAI"));
	StateTreeAI->SetStartLogicAutomatically(false);
	AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
	SetPerceptionComponent(*AIPerception);
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
}

void ALRGuardAIController::BeginPlay()
{
	Super::BeginPlay();
	const UGameInstance* gameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const ULRGameInstanceSubsystem* subsystem = gameInstance
		? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	if (subsystem && subsystem->GetTuningSet())
	{
		Tuning = subsystem->GetTuningSet()->Guard;
		StateTuning = subsystem->GetTuningSet()->State;
	}
	if (!ensureMsgf(Tuning && StateTuning, TEXT("%s requires Guard and State tuning."), *GetNameSafe(this)))
	{
		return;
	}
	ConfigurePerception();
	AIPerception->OnTargetPerceptionUpdated.AddDynamic(this, &ALRGuardAIController::HandlePerception);
	FTimerManager& timers = GetWorld()->GetTimerManager();
	timers.SetTimer(CaptureTimer, this, &ALRGuardAIController::HandleCaptureTimer,
		Tuning->CaptureCheckIntervalSeconds, true);
	timers.SetTimer(DetectionSampleTimer, this, &ALRGuardAIController::HandleDetectionSample,
		Tuning->DetectionSampleIntervalSeconds, true);
	LastDetectionSampleTime = GetWorld()->GetTimeSeconds();
	CachedAwareness = GetAwarenessSnapshot();
}

void ALRGuardAIController::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	if (AIPerception)
	{
		AIPerception->OnTargetPerceptionUpdated.RemoveDynamic(this, &ALRGuardAIController::HandlePerception);
	}
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(CaptureTimer);
		GetWorld()->GetTimerManager().ClearTimer(DetectionSampleTimer);
		GetWorld()->GetTimerManager().ClearTimer(StunTimer);
	}
	Super::EndPlay(endPlayReason);
}

void ALRGuardAIController::OnPossess(APawn* inPawn)
{
	Super::OnPossess(inPawn);
	ALRGuardCharacter* guard = Cast<ALRGuardCharacter>(inPawn);
	Alert = guard ? guard->GetAlertComponent() : nullptr;
	Knowledge = guard ? guard->GetKnowledgeComponent() : nullptr;
	if (!ensureMsgf(Alert.IsValid() && Knowledge.IsValid(), TEXT("%s requires Alert and Knowledge."),
		*GetNameSafe(guard)))
	{
		return;
	}
	Alert->OnDecayRequested.AddUObject(this, &ALRGuardAIController::HandleAlertDecayRequested);
	if (ULRCourageResponseComponent* courage = guard->GetCourageResponseComponent())
	{
		courage->OnKnockbackApplied.AddDynamic(this, &ALRGuardAIController::HandleKnockback);
	}
	ULRGuardDefinition* definition = guard->GetDefinition();
	if (definition && definition->Behavior)
	{
		StateTreeAI->SetStateTree(definition->Behavior);
		if (!StateTreeAI->IsRunning())
		{
			StateTreeAI->StartLogic();
		}
	}
	else
	{
		UE_LOG(LogLostRunicAI, Warning, TEXT("Guard=%s definition or Behavior StateTree is missing; using fallback."),
			*GetNameSafe(guard));
	}
	CachedAwareness = GetAwarenessSnapshot();
}

void ALRGuardAIController::OnUnPossess()
{
	if (Alert.IsValid())
	{
		Alert->OnDecayRequested.RemoveAll(this);
	}
	if (ALRGuardCharacter* guard = Cast<ALRGuardCharacter>(GetPawn()))
	{
		if (ULRCourageResponseComponent* courage = guard->GetCourageResponseComponent())
		{
			courage->OnKnockbackApplied.RemoveDynamic(this, &ALRGuardAIController::HandleKnockback);
		}
	}
	PerceivedSightContact.Reset();
	LastDetectionSampleTime = 0.0;
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(StunTimer);
	}
	if (StateTreeAI->IsRunning())
	{
		StateTreeAI->StopLogic(TEXT("OnUnPossess"));
	}
	Alert.Reset();
	Knowledge.Reset();
	CachedAwareness = FLRGuardAwarenessSnapshot();
	Super::OnUnPossess();
}

FLRGuardAwarenessSnapshot ALRGuardAIController::GetAwarenessSnapshot() const
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

ELRGuardBehaviorState ALRGuardAIController::GetResolvedBehavior() const
{
	return GetAwarenessSnapshot().ResolvedBehavior;
}

void ALRGuardAIController::ReceiveNoiseStimulus(const FLRGuardNoiseStimulus& stimulus)
{
	if (!Alert.IsValid() || !Knowledge.IsValid() || !stimulus.Reason.IsValid())
	{
		return;
	}
	const FLRGuardAwarenessSnapshot previous = GetAwarenessSnapshot();
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
	Knowledge->CommitAcceptedNoise(stimulus, confirmed && confirmed == stimulus.Source.Get(),
		GetEffectiveTuning().InvestigationRetargetDistance);
	CommitAwareness(previous, previousLevel, stimulus.Reason);
}

void ALRGuardAIController::MarkInvestigationReached()
{
	if (!Alert.IsValid() || !Knowledge.IsValid())
	{
		return;
	}
	const FLRGuardAwarenessSnapshot previous = GetAwarenessSnapshot();
	const int32 previousLevel = Alert->GetAlertLevel();
	Knowledge->MarkInvestigationReached();
	Alert->MarkInvestigationReached();
	CommitAwareness(previous, previousLevel, LRGameplayTags::SearchReached, true);
}

void ALRGuardAIController::ResetSearch()
{
	if (!Alert.IsValid() || !Knowledge.IsValid())
	{
		return;
	}
	const FLRGuardAwarenessSnapshot previous = GetAwarenessSnapshot();
	const int32 previousLevel = Alert->GetAlertLevel();
	Alert->ResetAfterSearch();
	Knowledge->ResetAwareness();
	PerceivedSightContact.Reset();
	CommitAwareness(previous, previousLevel, LRGameplayTags::SearchTimeout, true);
}

void ALRGuardAIController::HandleAlertDecayRequested()
{
	if (!Alert.IsValid() || !Knowledge.IsValid())
	{
		return;
	}
	const FLRGuardAwarenessSnapshot previous = GetAwarenessSnapshot();
	if (!LRAlertRules::ShouldDecay(previous.Alert, previous.Knowledge, Alert->IsObserving(),
		previous.ResolvedBehavior))
	{
		return;
	}
	const int32 previousLevel = Alert->GetAlertLevel();
	Alert->ApplyDelta(-GetEffectiveTuning().AlertDecayAmount);
	if (Alert->GetAlertLevel() == 0)
	{
		Knowledge->ResetAwareness();
		PerceivedSightContact.Reset();
	}
	CommitAwareness(previous, previousLevel, LRGameplayTags::SearchAlertDecay);
}

void ALRGuardAIController::CommitAwareness(const FLRGuardAwarenessSnapshot& previous,
	const int32 previousAlertLevel, const FGameplayTag reason, const bool bForcePublish)
{
	const FLRGuardAwarenessSnapshot current = GetAwarenessSnapshot();
	const bool bBehaviorChanged = previous.ResolvedBehavior != current.ResolvedBehavior;
	const bool bSignificant = bForcePublish || previous.Alert.Level != current.Alert.Level
		|| bBehaviorChanged || previous.Knowledge.Stage != current.Knowledge.Stage
		|| previous.Knowledge.bPendingThreatInvestigation != current.Knowledge.bPendingThreatInvestigation
		|| previous.Knowledge.InvestigationContextRevision != current.Knowledge.InvestigationContextRevision
		|| previous.Knowledge.ConfirmedThreat != current.Knowledge.ConfirmedThreat
		|| previous.Knowledge.VisualCandidate != current.Knowledge.VisualCandidate
		|| previous.Knowledge.CurrentVisibility.IsActive() != current.Knowledge.CurrentVisibility.IsActive();

	if (bBehaviorChanged)
	{
		if (StateTreeAI->IsRunning())
		{
			StateTreeAI->SendStateTreeEvent(LRGameplayTags::AIEventBehaviorChanged, FConstStructView(), FName());
		}
		else
		{
			EnterBehavior(current.ResolvedBehavior);
		}
	}
	else
	{
		RefreshBehaviorContext(previous, current);
	}
	CachedAwareness = current;
	Knowledge->PublishIfChanged(previous.Knowledge);
	if (bSignificant)
	{
		Alert->PublishCommittedChange(previousAlertLevel, current.ResolvedBehavior, reason,
			current.InvestigationLocation);
		OnGuardAwarenessChanged.Broadcast(current);
	}
}

const ULRGuardTuning& ALRGuardAIController::GetEffectiveTuning() const
{
	return Tuning ? *Tuning : *GetDefault<ULRGuardTuning>();
}
