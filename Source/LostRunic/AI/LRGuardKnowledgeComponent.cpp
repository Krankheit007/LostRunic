/**
 * @file LRGuardKnowledgeComponent.cpp
 * @brief 实现 Guard 感知记忆和视觉/噪声入口。
 */
#include "AI/LRGuardKnowledgeComponent.h"

#include "GameFramework/Actor.h"

ULRGuardKnowledgeComponent::ULRGuardKnowledgeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULRGuardKnowledgeComponent::SetVisualCandidate(AActor* candidate)
{
	Snapshot.VisualCandidate = candidate;
	Snapshot.bHasVisualCandidate = IsValid(candidate);
}

void ULRGuardKnowledgeComponent::RecordVisibleThreat(AActor* actor, const FVector& location)
{
	if (!IsValid(actor))
	{
		return;
	}

	Snapshot.VisualCandidate = actor;
	Snapshot.bHasVisualCandidate = true;
	Snapshot.bCurrentlyVisible = true;
	Snapshot.LastKnownThreatLocation = location;
	Snapshot.bHasLastKnownThreatLocation = true;
	Snapshot.LatestInvestigationLocation = location;
	Snapshot.bHasLatestInvestigationLocation = true;
}

void ULRGuardKnowledgeComponent::RecordSightLost(const FVector& lastKnownLocation)
{
	if (Snapshot.bHasVisualCandidate || Snapshot.bHasConfirmedThreat)
	{
		Snapshot.LastKnownThreatLocation = lastKnownLocation;
		Snapshot.bHasLastKnownThreatLocation = true;
	}

	Snapshot.bCurrentlyVisible = false;
	if (Snapshot.bHasLastKnownThreatLocation)
	{
		Snapshot.LatestInvestigationLocation = Snapshot.LastKnownThreatLocation;
		Snapshot.bHasLatestInvestigationLocation = true;
	}
}

void ULRGuardKnowledgeComponent::RecordDisturbance(const FLRGuardNoiseStimulus& stimulus,
	const bool bAllowInvestigationRetarget)
{
	Snapshot.LastDisturbanceLocation = stimulus.Location;
	Snapshot.bHasLastDisturbanceLocation = true;
	Snapshot.LastAcceptedStimulusSource = stimulus.Source;
	Snapshot.LastAcceptedStimulusReason = stimulus.Reason;
	Snapshot.LastAcceptedStimulusTimeSeconds = stimulus.TimeSeconds;

	if (bAllowInvestigationRetarget && !Snapshot.bCurrentlyVisible)
	{
		Snapshot.LatestInvestigationLocation = stimulus.Location;
		Snapshot.bHasLatestInvestigationLocation = true;
	}
}

void ULRGuardKnowledgeComponent::SetConfirmedThreat(AActor* threat, const FVector& lastKnownLocation)
{
	if (!IsValid(threat))
	{
		return;
	}

	Snapshot.ConfirmedThreat = threat;
	Snapshot.bHasConfirmedThreat = true;
	Snapshot.LastKnownThreatLocation = lastKnownLocation;
	Snapshot.bHasLastKnownThreatLocation = true;
	Snapshot.LatestInvestigationLocation = lastKnownLocation;
	Snapshot.bHasLatestInvestigationLocation = true;
}

void ULRGuardKnowledgeComponent::SuspendVisualContact()
{
	Snapshot.bCurrentlyVisible = false;
	Snapshot.VisualCandidate.Reset();
	Snapshot.bHasVisualCandidate = false;
	if (Snapshot.bHasLastKnownThreatLocation)
	{
		Snapshot.LatestInvestigationLocation = Snapshot.LastKnownThreatLocation;
		Snapshot.bHasLatestInvestigationLocation = true;
	}
}

void ULRGuardKnowledgeComponent::ResetAwareness()
{
	Snapshot = FLRGuardKnowledgeSnapshot();
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
		&& left.LatestInvestigationLocation.Equals(right.LatestInvestigationLocation)
		&& left.bHasLatestInvestigationLocation == right.bHasLatestInvestigationLocation
		&& left.bCurrentlyVisible == right.bCurrentlyVisible
		&& left.LastAcceptedStimulusSource == right.LastAcceptedStimulusSource
		&& left.LastAcceptedStimulusReason == right.LastAcceptedStimulusReason
		&& FMath::IsNearlyEqual(left.LastAcceptedStimulusTimeSeconds,
			right.LastAcceptedStimulusTimeSeconds);
}
