/**
 * @file LRGuardKnowledgeComponent.h
 * @brief 保存 Guard 感知记忆，不访问 World、Perception、LOS 或 Navigation。
 */
#pragma once

#include "AI/LRGuardTypes.h"
#include "Components/ActorComponent.h"

#include "LRGuardKnowledgeComponent.generated.h"

class ALRGuardAIController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRGuardKnowledgeChanged,
	const FLRGuardKnowledgeSnapshot&, snapshot);

/** Guard 感知事实的唯一运行时所有者。 */
UCLASS(ClassGroup = "Lost Runic", BlueprintType, meta = (BlueprintSpawnableComponent, DisplayName = "Lost Runic Guard Knowledge"))
class LOSTRUNIC_API ULRGuardKnowledgeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULRGuardKnowledgeComponent();

	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Knowledge")
	FLRGuardKnowledgeSnapshot GetSnapshot() const { return Snapshot; }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Knowledge")
	bool IsCurrentlyVisible() const { return Snapshot.bCurrentlyVisible; }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Knowledge")
	bool HasVisualCandidate() const { return Snapshot.bHasVisualCandidate; }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Knowledge")
	bool HasConfirmedThreat() const { return Snapshot.bHasConfirmedThreat; }

	bool HasConfirmedThreatActor(const AActor* actor) const
	{
		return actor && Snapshot.bHasConfirmedThreat && Snapshot.ConfirmedThreat.Get() == actor;
	}

	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Knowledge")
	FVector GetLastKnownThreatLocation() const { return Snapshot.LastKnownThreatLocation; }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Knowledge")
	FVector GetLastDisturbanceLocation() const { return Snapshot.LastDisturbanceLocation; }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|AI|Knowledge")
	FVector GetLatestInvestigationLocation() const { return Snapshot.LatestInvestigationLocation; }

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|AI|Knowledge")
	FLRGuardKnowledgeChanged OnKnowledgeChanged;

private:
	friend class ALRGuardAIController;
#if WITH_DEV_AUTOMATION_TESTS
	friend class FLRGuardSightContactLifecycleRuntimeTest;
	friend class FLRGuardKnowledgeMemoryTest;
#endif

	void SetVisualCandidate(AActor* candidate);
	void RecordVisibleThreat(AActor* actor, const FVector& location);
	void RecordSightLost(const FVector& lastKnownLocation);
	void RecordDisturbance(const FLRGuardNoiseStimulus& stimulus, bool bAllowInvestigationRetarget);
	void SetConfirmedThreat(AActor* threat, const FVector& lastKnownLocation);
	void SuspendVisualContact();
	void ResetAwareness();

	void PublishIfChanged(const FLRGuardKnowledgeSnapshot& previousSnapshot);
	static bool AreSnapshotsEqual(const FLRGuardKnowledgeSnapshot& left,
		const FLRGuardKnowledgeSnapshot& right);

	UPROPERTY(Transient)
	FLRGuardKnowledgeSnapshot Snapshot;
};
