/**
 * @file LRGuardKnowledgeComponent.h
 * @brief Owns controller-fed guard knowledge without world, perception, LOS or navigation queries.
 */
#pragma once

#include "AI/LRGuardTypes.h"
#include "Components/ActorComponent.h"

#include "LRGuardKnowledgeComponent.generated.h"

class ALRGuardAIController;
class ULRGuardTuning;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRGuardKnowledgeChanged,
	const FLRGuardKnowledgeSnapshot&, snapshot);

UCLASS(ClassGroup = "Lost Runic", BlueprintType, meta = (BlueprintSpawnableComponent, DisplayName = "Lost Runic Guard Knowledge"))
class LOSTRUNIC_API ULRGuardKnowledgeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULRGuardKnowledgeComponent();

	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Knowledge")
	FLRGuardKnowledgeSnapshot GetSnapshot() const { return Snapshot; }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Knowledge")
	FLRGuardVisibilityResult GetCurrentVisibility() const { return Snapshot.CurrentVisibility; }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Knowledge")
	ELRGuardDetectionStage GetDetectionStage() const { return Snapshot.Stage; }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Knowledge")
	float GetEffectiveExposureSeconds() const { return Snapshot.EffectiveExposureSeconds; }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Knowledge")
	bool HasVisualCandidate() const { return Snapshot.bHasVisualCandidate; }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Knowledge")
	bool HasConfirmedThreat() const { return Snapshot.bHasConfirmedThreat; }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Knowledge")
	bool HasPendingThreatInvestigation() const { return Snapshot.bPendingThreatInvestigation; }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Knowledge")
	FVector GetLastKnownThreatLocation() const { return Snapshot.LastKnownThreatLocation; }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Knowledge")
	FVector GetLastDisturbanceLocation() const { return Snapshot.LastDisturbanceLocation; }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Knowledge")
	int32 GetInvestigationContextRevision() const { return Snapshot.InvestigationContextRevision; }

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|AI|Knowledge")
	FLRGuardKnowledgeChanged OnKnowledgeChanged;

private:
	friend class ALRGuardAIController;

	void SetVisualCandidate(AActor* candidate);
	void ClearVisualCandidate();
	void ApplyVisibilitySample(const FLRGuardVisibilityResult& sample, float deltaSeconds,
		const ULRGuardTuning& tuning);
	void RecordVisualEvidence(AActor* actor, const FVector& location, bool bPendingInvestigation,
		float retargetDistance);
	void SetConfirmedThreat(AActor* threat, const FVector& lastKnownLocation, bool bLatch = true);
	void RecordSightLoss(const FVector& lastKnownLocation);
	void CommitAcceptedNoise(const FLRGuardNoiseStimulus& stimulus, bool bConfirmedThreatSource,
		float retargetDistance);
	void MarkInvestigationReached();
	void ResetAwareness();
	void PublishIfChanged(const FLRGuardKnowledgeSnapshot& previousSnapshot);
	static bool AreSnapshotsEqual(const FLRGuardKnowledgeSnapshot& left,
		const FLRGuardKnowledgeSnapshot& right);

	UPROPERTY(Transient)
	FLRGuardKnowledgeSnapshot Snapshot;

	bool bThreatLatched = false;
};
