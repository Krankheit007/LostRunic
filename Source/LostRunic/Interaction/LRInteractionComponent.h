#pragma once

#include "Components/ActorComponent.h"
#include "Interaction/LRInteractionTypes.h"

#include "LRInteractionComponent.generated.h"

class ULRInteractionTuning;
class ULRInventoryComponent;
class ULRStateComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FLRInteractionTargetChanged, AActor*, target,
	FLRInteractionOption, option, ELRInteractionRange, range);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRInteractionExecuted, FLRInteractionResult, result);

/** Selects one nearest interactable on a tuned timer and owns interaction execution. */
UCLASS(ClassGroup = "Lost Runic", BlueprintType, meta = (BlueprintSpawnableComponent, DisplayName = "Lost Runic Interaction"))
class LOSTRUNIC_API ULRInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULRInteractionComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Interaction")
	FLRInteractionResult PerformPrimaryInteraction();

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Interaction")
	AActor* GetCurrentTarget() const { return CurrentTarget.Get(); }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Interaction")
	FLRInteractionOption GetCurrentOption() const { return CurrentOption; }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Interaction")
	ELRInteractionRange GetCurrentRange() const { return CurrentRange; }

	void LogDiagnostics() const;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Interaction")
	FLRInteractionTargetChanged OnTargetChanged;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Interaction")
	FLRInteractionExecuted OnInteractionExecuted;

private:
	struct FCandidate
	{
		TWeakObjectPtr<AActor> Actor;
		FLRInteractionOption Option;
		FLRInteractionCandidateScore Score;
		float ExecuteDistance = 0.0f;
	};

	void ScanCandidates();
	bool IsOccluded(AActor* target, const FVector& targetLocation) const;
	void ApplySelection(const TArray<FCandidate>& candidates, int32 selectedIndex);
	const ULRInteractionTuning& GetEffectiveTuning() const;

	UPROPERTY(Transient)
	TObjectPtr<ULRInteractionTuning> Tuning;

	UPROPERTY(Transient)
	TObjectPtr<ULRInventoryComponent> Inventory;

	UPROPERTY(Transient)
	TObjectPtr<ULRStateComponent> State;

	TWeakObjectPtr<AActor> CurrentTarget;
	FLRInteractionOption CurrentOption;
	ELRInteractionRange CurrentRange = ELRInteractionRange::None;
	FTimerHandle QueryTimer;
};
