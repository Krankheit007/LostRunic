/**
 * @file LRGuardKnowledgeComponent.cpp
 * @brief Implements transactional, controller-fed Guard Knowledge.
 */
#include "AI/LRGuardKnowledgeComponent.h"

#include "AI/LRGuardPerceptionRules.h"
#include "Data/LRGuardTuning.h"
#include "GameFramework/Actor.h"

namespace
{
	bool AreVisibilityResultsEqual(const FLRGuardVisibilityResult& left,
		const FLRGuardVisibilityResult& right)
	{
		return left.bRangeGate == right.bRangeGate && left.bConeGate == right.bConeGate
			&& left.bLOSGate == right.bLOSGate && left.bValidContactGate == right.bValidContactGate
			&& left.bHardVisibilityGate == right.bHardVisibilityGate
			&& FMath::IsNearlyEqual(left.DistanceFactor, right.DistanceFactor)
			&& FMath::IsNearlyEqual(left.MovementFactor, right.MovementFactor)
			&& FMath::IsNearlyEqual(left.ExposureFactor, right.ExposureFactor)
			&& FMath::IsNearlyEqual(left.LightingFactor, right.LightingFactor)
			&& FMath::IsNearlyEqual(left.PostureFactor, right.PostureFactor)
			&& FMath::IsNearlyEqual(left.VisibilityScore, right.VisibilityScore);
	}
}

ULRGuardKnowledgeComponent::ULRGuardKnowledgeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULRGuardKnowledgeComponent::SetVisualCandidate(AActor* candidate)
{
	Snapshot.VisualCandidate = candidate;
	Snapshot.bHasVisualCandidate = IsValid(candidate);
}

void ULRGuardKnowledgeComponent::ClearVisualCandidate()
{
	Snapshot.VisualCandidate.Reset();
	Snapshot.bHasVisualCandidate = false;
	Snapshot.CurrentVisibility = FLRGuardVisibilityResult();
}

void ULRGuardKnowledgeComponent::ApplyVisibilitySample(const FLRGuardVisibilityResult& sample, const float deltaSeconds, const ULRGuardTuning& tuning)
{
	Snapshot.CurrentVisibility = sample;
	Snapshot.EffectiveExposureSeconds = LRGuardPerceptionRules::IntegrateDetectionExposure(
		Snapshot.EffectiveExposureSeconds, sample, deltaSeconds, tuning);
	if (Snapshot.EffectiveExposureSeconds <= KINDA_SMALL_NUMBER)
	{
		Snapshot.EffectiveExposureSeconds = 0.0f;
	}
	Snapshot.Stage = LRGuardPerceptionRules::ResolveDetectionStage(
		Snapshot.EffectiveExposureSeconds, tuning);
}

void ULRGuardKnowledgeComponent::RecordVisualEvidence(AActor* actor, const FVector& location, const bool bPendingInvestigation)
{
	Snapshot.VisualCandidate = actor;
	Snapshot.bHasVisualCandidate = IsValid(actor);
	Snapshot.LastKnownThreatLocation = location;
	Snapshot.bHasLastKnownThreatLocation = Snapshot.bHasVisualCandidate;
	Snapshot.bPendingThreatInvestigation = bPendingInvestigation;
}

void ULRGuardKnowledgeComponent::SetConfirmedThreat(AActor* threat, const FVector& lastKnownLocation, const bool bLatch)
{
	Snapshot.ConfirmedThreat = threat;
	Snapshot.bHasConfirmedThreat = IsValid(threat);
	if (Snapshot.bHasConfirmedThreat)
	{
		Snapshot.LastKnownThreatLocation = lastKnownLocation;
		Snapshot.bHasLastKnownThreatLocation = true;
		Snapshot.bPendingThreatInvestigation = false;
	}
	bThreatLatched = bLatch && Snapshot.bHasConfirmedThreat;
}

void ULRGuardKnowledgeComponent::RecordSightLoss(const FVector& lastKnownLocation)
{
	const bool bHadEvidence = Snapshot.bHasConfirmedThreat || Snapshot.bHasVisualCandidate
		|| Snapshot.bHasLastKnownThreatLocation;
	Snapshot.LastKnownThreatLocation = lastKnownLocation;
	Snapshot.bHasLastKnownThreatLocation = bHadEvidence;
	ClearVisualCandidate();
	if (!bThreatLatched || !Snapshot.ConfirmedThreat.IsValid())
	{
		Snapshot.ConfirmedThreat.Reset();
		Snapshot.bHasConfirmedThreat = false;
		bThreatLatched = false;
	}
	Snapshot.bPendingThreatInvestigation = bHadEvidence;
}

void ULRGuardKnowledgeComponent::CommitAcceptedNoise(const FLRGuardNoiseStimulus& stimulus, const bool bConfirmedThreatSource)
{
	if (bConfirmedThreatSource)
	{
		Snapshot.LastKnownThreatLocation = stimulus.Location;
		Snapshot.bHasLastKnownThreatLocation = true;
		Snapshot.bPendingThreatInvestigation = true;
	}
	else
	{
		Snapshot.LastDisturbanceLocation = stimulus.Location;
		Snapshot.bHasLastDisturbanceLocation = true;
	}
	Snapshot.LastAcceptedStimulusSource = stimulus.Source;
	Snapshot.LastAcceptedStimulusReason = stimulus.Reason;
	Snapshot.LastAcceptedStimulusTimeSeconds = stimulus.TimeSeconds;
}

void ULRGuardKnowledgeComponent::MarkInvestigationReached()
{
	Snapshot.bPendingThreatInvestigation = false;
}

void ULRGuardKnowledgeComponent::MarkInvestigationUnreachable()
{
	Snapshot.bPendingThreatInvestigation = false;
}

void ULRGuardKnowledgeComponent::AdvanceInvestigationContextRevision()
{
	++Snapshot.InvestigationContextRevision;
}

void ULRGuardKnowledgeComponent::SuspendVisualContact()
{
	ClearVisualCandidate();
	Snapshot.EffectiveExposureSeconds = 0.0f;
	Snapshot.Stage = ELRGuardDetectionStage::None;
}

void ULRGuardKnowledgeComponent::ResetAwareness()
{
	Snapshot = FLRGuardKnowledgeSnapshot();
	bThreatLatched = false;
}

void ULRGuardKnowledgeComponent::PublishIfChanged(const FLRGuardKnowledgeSnapshot& previousSnapshot)
{
	if (!AreSnapshotsEqual(previousSnapshot, Snapshot))
	{
		OnKnowledgeChanged.Broadcast(Snapshot);
	}
}

bool ULRGuardKnowledgeComponent::AreSnapshotsEqual(const FLRGuardKnowledgeSnapshot& left,
	const FLRGuardKnowledgeSnapshot& right)
{
	return left.VisualCandidate == right.VisualCandidate
		&& left.bHasVisualCandidate == right.bHasVisualCandidate
		&& left.ConfirmedThreat == right.ConfirmedThreat
		&& left.bHasConfirmedThreat == right.bHasConfirmedThreat
		&& left.LastKnownThreatLocation.Equals(right.LastKnownThreatLocation)
		&& left.bHasLastKnownThreatLocation == right.bHasLastKnownThreatLocation
		&& left.LastDisturbanceLocation.Equals(right.LastDisturbanceLocation)
		&& left.bHasLastDisturbanceLocation == right.bHasLastDisturbanceLocation
		&& AreVisibilityResultsEqual(left.CurrentVisibility, right.CurrentVisibility)
		&& left.bPendingThreatInvestigation == right.bPendingThreatInvestigation
		&& FMath::IsNearlyEqual(left.EffectiveExposureSeconds, right.EffectiveExposureSeconds)
		&& left.Stage == right.Stage
		&& left.LastAcceptedStimulusSource == right.LastAcceptedStimulusSource
		&& left.LastAcceptedStimulusReason == right.LastAcceptedStimulusReason
		&& FMath::IsNearlyEqual(left.LastAcceptedStimulusTimeSeconds, right.LastAcceptedStimulusTimeSeconds)
		&& left.InvestigationContextRevision == right.InvestigationContextRevision;
}
