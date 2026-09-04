#pragma once

#include "Components/ActorComponent.h"
#include "UObject/ObjectKey.h"

#include "LRCameraCutawayComponent.generated.h"

class ACharacter;
class ALRHidePoint;
class UCameraComponent;
class ULRHideComponent;
class ULRCutawayTargetComponent;
class UMaterialParameterCollectionInstance;
class UPrimitiveComponent;

namespace LR::Cutaway
{
	/** Candidate ordering used to fill an empty sticky suppression slot. */
	struct LOSTRUNIC_API FStickyCandidate
	{
		bool bActive = false;
		float Amount = 0.0f;
		float DistanceSquared = TNumericLimits<float>::Max();
		FObjectKey ObjectKey;
	};

	/** Active requests win, then amount, camera distance, and a stable object key. */
	LOSTRUNIC_API bool IsStickyCandidateHigherPriority(const FStickyCandidate& left,
		const FStickyCandidate& right);
}

/** Detects camera occluders and writes only the per-frame Local hole center. */
UCLASS(ClassGroup = "Lost Runic", BlueprintType, meta = (BlueprintSpawnableComponent, DisplayName = "Lost Runic Camera Cutaway"))
class LOSTRUNIC_API ULRCameraCutawayComponent : public UActorComponent
{
	GENERATED_BODY()

	friend class FLRCutawayStickyPriorityTest;

public:
	ULRCameraCutawayComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
	virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* tickFunction) override;

private:
	// PP suppression capacity only: overflow targets still cut away through their own CPD.
	static constexpr int32 MaxStickySlots = 4;
	static constexpr float StickyReleaseThreshold = 0.02f;

	void DetectOccluders();
	void CollectTraceTargets(const TArray<FHitResult>& hits, TSet<ULRCutawayTargetComponent*>& outTargets) const;
	void UpdateLocalRequests(const TSet<ULRCutawayTargetComponent*>& newTargets);
	void UpdateStickySlots();
	void RefreshStickySlotAmounts();
	void ReleaseStickySlot(int32 slotIndex);
	bool InitializeSuppressionParameters();
	void ClearPublishedSuppressionStates();
	void PublishSuppressionStates();
	bool HasStickySlots() const;
	bool IsStickyTarget(const ULRCutawayTargetComponent* target) const;
	void ClearLocalRequests();
	void UpdateProjectedCenter();
	void RegisterCharacterVisualPrimitives();
	FVector GetChestLocation() const;

	UFUNCTION()
	void HandleHiddenStateChanged(bool bHidden, ALRHidePoint* hidePoint);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cutaway|Detection", meta = (AllowPrivateAccess = "true", ClampMin = "1.0", ClampMax = "60.0", Units = "Hz"))
	float DetectionFrequencyHz = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cutaway|Detection", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "100.0", Units = "cm"))
	float PrimarySphereRadiusCm = 20.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cutaway|Local Hole", meta = (AllowPrivateAccess = "true", ClampMin = "32.0", ClampMax = "600.0"))
	float RadiusRefPx = 200.0f;

	UPROPERTY(Transient)
	TObjectPtr<ACharacter> Character;

	UPROPERTY(Transient)
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(Transient)
	TObjectPtr<ULRHideComponent> HideComponent;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULRCutawayTargetComponent>> UpdatingTargets;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPrimitiveComponent>> RegisteredVisualPrimitives;

	/** Four stable per-camera memberships; null entries are compacted before publishing. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<ULRCutawayTargetComponent>> StickySlotTargets;

	UPROPERTY(Transient)
	TArray<FVector2D> StickySlotCenters;

	UPROPERTY(Transient)
	TArray<float> StickySlotRadii;

	UPROPERTY(Transient)
	TArray<float> StickySlotAmounts;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialParameterCollectionInstance> CutawayViewParameterCollectionInstance;

	TSet<TWeakObjectPtr<ULRCutawayTargetComponent>> RequestedTargets;
	FTimerHandle DetectionTimer;
	bool bHardHidden = false;
	bool bSuppressionWarningLogged = false;
};
