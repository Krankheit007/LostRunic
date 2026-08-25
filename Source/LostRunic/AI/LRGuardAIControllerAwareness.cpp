/**
 * @file LRGuardAIControllerAwareness.cpp
 * @brief Commits final Guard Awareness snapshots and coordinates transactional behavior transitions.
 */
#include "AI/LRGuardAIController.h"

#include "AI/LRAlertComponent.h"
#include "AI/LRAlertRules.h"
#include "AI/LRGuardKnowledgeComponent.h"
#include "Components/StateTreeAIComponent.h"
#include "Core/LRGameplayTags.h"
#include "Data/LRGuardTuning.h"

void ALRGuardAIController::CommitAwareness(const FGameplayTag reason, const bool bForcePublish)
{
	if (!Alert.IsValid() || !Knowledge.IsValid())
	{
		return;
	}
	const FLRGuardAwarenessSnapshot previous = CachedAwareness;
	const FLRGuardAwarenessSnapshot current = GetAwarenessSnapshot();
	const bool bThreatChanged = previous.Knowledge.ConfirmedThreat != current.Knowledge.ConfirmedThreat
		|| previous.Knowledge.bHasConfirmedThreat != current.Knowledge.bHasConfirmedThreat;
	const bool bVisualContactChanged = previous.Knowledge.VisualCandidate != current.Knowledge.VisualCandidate
		|| previous.Knowledge.bHasVisualCandidate != current.Knowledge.bHasVisualCandidate
		|| previous.Knowledge.CurrentVisibility.IsActive() != current.Knowledge.CurrentVisibility.IsActive();
	const bool bSignificant = bForcePublish || previous.Alert.Level != current.Alert.Level
		|| previous.ResolvedBehavior != current.ResolvedBehavior
		|| previous.Knowledge.Stage != current.Knowledge.Stage
		|| previous.Knowledge.bPendingThreatInvestigation != current.Knowledge.bPendingThreatInvestigation
		|| previous.Knowledge.InvestigationContextRevision != current.Knowledge.InvestigationContextRevision
		|| bThreatChanged || bVisualContactChanged;

	CachedAwareness = current;
	Knowledge->PublishIfChanged(previous.Knowledge);
	if (bSignificant)
	{
		Alert->PublishCommittedChange(previous.Alert.Level, current.ResolvedBehavior, reason,
			current.InvestigationLocation);
		OnGuardAwarenessChanged.Broadcast(current);
	}
}

void ALRGuardAIController::ProcessAwarenessTransaction(const FGameplayTag reason, const bool bForcePublish)
{
	if (!Alert.IsValid() || !Knowledge.IsValid())
	{
		return;
	}
	if (bAwarenessCommitDeferred)
	{
		DeferredAwarenessReason = reason;
		bDeferredForcePublish = bDeferredForcePublish || bForcePublish;
		return;
	}

	const FLRGuardAwarenessSnapshot current = GetAwarenessSnapshot();
	if (current.ResolvedBehavior != CachedAwareness.ResolvedBehavior)
	{
		if (StateTreeAI && StateTreeAI->IsRunning())
		{
			DeferAwarenessCommit(reason, bForcePublish);
			StateTreeAI->SendStateTreeEvent(LRGameplayTags::AIEventBehaviorChanged, FConstStructView(), FName());
			return;
		}

		if (ActiveBehavior != current.ResolvedBehavior)
		{
			ExitBehavior(ActiveBehavior);
		}
		const ELRGuardBehaviorEntryResult entryResult = EnterBehavior(current.ResolvedBehavior);
		FGameplayTag finalReason = reason;
		bool bFinalForcePublish = bForcePublish;
		if (entryResult == ELRGuardBehaviorEntryResult::AlreadyAtGoal)
		{
			finalReason = LRGameplayTags::SearchReached;
			bFinalForcePublish = true;
		}
		else if (entryResult == ELRGuardBehaviorEntryResult::Failed)
		{
			finalReason = LRGameplayTags::SearchUnreachable;
			bFinalForcePublish = true;
		}
		const FLRGuardAwarenessSnapshot afterEntry = GetAwarenessSnapshot();
		if (afterEntry.ResolvedBehavior != current.ResolvedBehavior)
		{
			ExitBehavior(current.ResolvedBehavior);
			EnterBehavior(afterEntry.ResolvedBehavior);
		}
		CommitAwareness(finalReason, bFinalForcePublish);
		return;
	}

	RefreshBehaviorContext(current);
	const FLRGuardAwarenessSnapshot afterRefresh = GetAwarenessSnapshot();
	if (afterRefresh.ResolvedBehavior != CachedAwareness.ResolvedBehavior)
	{
		if (StateTreeAI && StateTreeAI->IsRunning())
		{
			DeferAwarenessCommit(reason, bForcePublish);
			StateTreeAI->SendStateTreeEvent(LRGameplayTags::AIEventBehaviorChanged, FConstStructView(), FName());
			return;
		}
		ExitBehavior(ActiveBehavior);
		EnterBehavior(afterRefresh.ResolvedBehavior);
	}
	CommitAwareness(reason, bForcePublish);
}

void ALRGuardAIController::FinalizeStateTreeBehaviorEntry(const ELRGuardBehaviorState behavior,
	const ELRGuardBehaviorEntryResult result)
{
	if (!bAwarenessCommitDeferred)
	{
		if (result != ELRGuardBehaviorEntryResult::Running && Alert.IsValid() && Knowledge.IsValid())
		{
			CommitAwareness(result == ELRGuardBehaviorEntryResult::AlreadyAtGoal
				? LRGameplayTags::SearchReached : LRGameplayTags::SearchUnreachable, true);
		}
		return;
	}

	FGameplayTag reason = DeferredAwarenessReason;
	const bool bForcePublish = bDeferredForcePublish;
	bAwarenessCommitDeferred = false;
	DeferredAwarenessReason = FGameplayTag();
	bDeferredForcePublish = false;
	if (result == ELRGuardBehaviorEntryResult::AlreadyAtGoal)
	{
		reason = LRGameplayTags::SearchReached;
	}
	else if (result == ELRGuardBehaviorEntryResult::Failed)
	{
		reason = LRGameplayTags::SearchUnreachable;
	}
	CommitAwareness(reason, bForcePublish);
}

void ALRGuardAIController::DeferAwarenessCommit(const FGameplayTag reason, const bool bForcePublish)
{
	bAwarenessCommitDeferred = true;
	DeferredAwarenessReason = reason;
	bDeferredForcePublish = bForcePublish;
}

void ALRGuardAIController::ApplyInvestigationReached()
{
	ClearInvestigationRetrySuppression();
	ClearInvestigationMoveRequest();
	Knowledge->MarkInvestigationReached();
	Alert->MarkInvestigationReached();
}

void ALRGuardAIController::ApplyInvestigationUnreachable()
{
	const FLRGuardAwarenessSnapshot awareness = GetAwarenessSnapshot();
	const bool bHadMoveTarget = bHasInvestigationMoveTarget;
	const FVector unreachableLocation = bHadMoveTarget
		? CurrentInvestigationMoveTarget : awareness.InvestigationLocation;
	bHasUnreachableInvestigationLocation = bHadMoveTarget
		|| awareness.Knowledge.bHasLastKnownThreatLocation
		|| awareness.Knowledge.bHasLastDisturbanceLocation;
	LastUnreachableInvestigationLocation = unreachableLocation;
	bInvestigationRetrySuppressed = true;
	ClearInvestigationMoveRequest();
	Knowledge->MarkInvestigationUnreachable();
	Alert->MarkInvestigationUnreachable();
}

void ALRGuardAIController::ClearInvestigationRetrySuppression()
{
	bInvestigationRetrySuppressed = false;
	bHasUnreachableInvestigationLocation = false;
	LastUnreachableInvestigationLocation = FVector::ZeroVector;
}

bool ALRGuardAIController::ShouldRetryInvestigationAt(const FVector& location) const
{
	if (!bInvestigationRetrySuppressed || !bHasUnreachableInvestigationLocation)
	{
		return true;
	}
	return FVector::DistSquared(LastUnreachableInvestigationLocation, location)
		>= FMath::Square(GetEffectiveTuning().InvestigationRetargetDistance);
}

void ALRGuardAIController::ClearInvestigationMoveRequest()
{
	InvestigationMoveRequestId = FAIRequestID::InvalidRequest;
	bHasInvestigationMoveTarget = false;
	CurrentInvestigationMoveTarget = FVector::ZeroVector;
}
