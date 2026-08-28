/**
 * @file LRGuardAIControllerAwareness.cpp
 * @brief 发布 Awareness 快照并驱动 StateTree 重新选择五个 Guard 行为。
 */
#include "AI/LRGuardAIController.h"

#include "AI/LRAlertComponent.h"
#include "AI/LRAlertRules.h"
#include "AI/LRGuardKnowledgeComponent.h"
#include "Components/StateTreeAIComponent.h"
#include "Core/LRGameplayTags.h"

namespace
{
	bool HasKnowledgeChanged(const FLRGuardKnowledgeSnapshot& previous,
		const FLRGuardKnowledgeSnapshot& current)
	{
		return previous.VisualCandidate != current.VisualCandidate
			|| previous.bHasVisualCandidate != current.bHasVisualCandidate
			|| previous.ConfirmedThreat != current.ConfirmedThreat
			|| previous.bHasConfirmedThreat != current.bHasConfirmedThreat
			|| previous.bCurrentlyVisible != current.bCurrentlyVisible
			|| previous.bHasLastKnownThreatLocation != current.bHasLastKnownThreatLocation
			|| !previous.LastKnownThreatLocation.Equals(current.LastKnownThreatLocation)
			|| previous.bHasLastDisturbanceLocation != current.bHasLastDisturbanceLocation
			|| !previous.LastDisturbanceLocation.Equals(current.LastDisturbanceLocation)
			|| previous.bHasLatestInvestigationLocation != current.bHasLatestInvestigationLocation
			|| !previous.LatestInvestigationLocation.Equals(current.LatestInvestigationLocation)
			|| previous.LastAcceptedStimulusSource != current.LastAcceptedStimulusSource
			|| previous.LastAcceptedStimulusReason != current.LastAcceptedStimulusReason
			|| !FMath::IsNearlyEqual(previous.LastAcceptedStimulusTimeSeconds,
				current.LastAcceptedStimulusTimeSeconds);
	}
}

void ALRGuardAIController::CommitAwareness(const FGameplayTag reason, const bool bForcePublish)
{
	if (!Alert.IsValid() || !Knowledge.IsValid())
	{
		return;
	}

	const FLRGuardAwarenessSnapshot previous = CachedAwareness;
	const FLRGuardAwarenessSnapshot current = BuildCurrentAwarenessSnapshot();

	if (current.Alert.Level == LRAlertRules::MinAlertLevel)
	{
		bSightToChaseGraceConsumed = false;
	}
	else if (current.Alert.Level >= LRAlertRules::SuspiciousMinLevel
		&& current.Alert.Level <= LRAlertRules::SuspiciousMaxLevel
		&& (previous.Alert.Level == LRAlertRules::MinAlertLevel
			|| previous.Alert.Level >= LRAlertRules::InvestigateMinLevel))
	{
		// 6 -> 5 是衰减退回白色，下一次 5 -> Sight -> 6 要重新获得一次 Grace。
		bSightToChaseGraceConsumed = false;
	}
	else if (current.Alert.Level >= LRAlertRules::InvestigateMinLevel
		&& previous.Alert.Level <= LRAlertRules::SuspiciousMaxLevel
		&& !bSightToChaseGraceActive)
	{
		// 非视觉事件跨入红色时，本轮红色周期没有 Sight Grace。
		bSightToChaseGraceConsumed = true;
	}

	const bool bSignificant = bForcePublish
		|| previous.Alert.Level != current.Alert.Level
		|| previous.ResolvedBehavior != current.ResolvedBehavior
		|| HasKnowledgeChanged(previous.Knowledge, current.Knowledge);

	CachedAwareness = current;
	Knowledge->PublishIfChanged(previous.Knowledge);
	if (bSignificant)
	{
		Alert->PublishCommittedChange(previous.Alert.Level, current.ResolvedBehavior, reason,
			current.InvestigationLocation);
		OnGuardAwarenessChanged.Broadcast(current);
	}
}

void ALRGuardAIController::ProcessAwarenessTransaction(const FGameplayTag reason,
	const bool bForcePublish)
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

	const FLRGuardAwarenessSnapshot current = BuildCurrentAwarenessSnapshot();
	if (current.ResolvedBehavior != CachedAwareness.ResolvedBehavior)
	{
		if (StateTreeAI && StateTreeAI->IsRunning())
		{
			DeferAwarenessCommit(reason, bForcePublish);
			StateTreeAI->SendStateTreeEvent(LRGameplayTags::AIEventBehaviorChanged,
				FConstStructView(), FName());
			return;
		}

		if (ActiveBehavior != current.ResolvedBehavior)
		{
			ExitBehavior(ActiveBehavior);
		}
		EnterBehavior(current.ResolvedBehavior);
		CommitAwareness(reason, bForcePublish);
		return;
	}

	RefreshBehaviorContext(current);
	const FLRGuardAwarenessSnapshot afterRefresh = BuildCurrentAwarenessSnapshot();
	if (afterRefresh.ResolvedBehavior != CachedAwareness.ResolvedBehavior)
	{
		if (StateTreeAI && StateTreeAI->IsRunning())
		{
			DeferAwarenessCommit(reason, bForcePublish);
			StateTreeAI->SendStateTreeEvent(LRGameplayTags::AIEventBehaviorChanged,
				FConstStructView(), FName());
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
	(void)behavior;
	(void)result;
	if (!bAwarenessCommitDeferred)
	{
		return;
	}

	const FGameplayTag reason = DeferredAwarenessReason;
	const bool bForcePublish = bDeferredForcePublish;
	bAwarenessCommitDeferred = false;
	DeferredAwarenessReason = FGameplayTag();
	bDeferredForcePublish = false;
	RefreshBehaviorContext(BuildCurrentAwarenessSnapshot());
	CommitAwareness(reason, bForcePublish);
}

void ALRGuardAIController::DeferAwarenessCommit(const FGameplayTag reason,
	const bool bForcePublish)
{
	bAwarenessCommitDeferred = true;
	DeferredAwarenessReason = reason;
	bDeferredForcePublish = bForcePublish;
}
