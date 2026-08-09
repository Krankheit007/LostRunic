#include "Interaction/LRInteractionComponent.h"

#include "Core/LRGameplayTags.h"
#include "Core/LRLog.h"
#include "Data/LRGameTuningSet.h"
#include "Data/LRInteractionTuning.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "Interaction/LRInteractable.h"
#include "Interaction/LRInteractionRules.h"
#include "Items/LRInventoryComponent.h"
#include "State/LRStateComponent.h"
#include "EngineUtils.h"
#include "TimerManager.h"

ULRInteractionComponent::ULRInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULRInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
	Inventory = GetOwner() ? GetOwner()->FindComponentByClass<ULRInventoryComponent>() : nullptr;
	State = GetOwner() ? GetOwner()->FindComponentByClass<ULRStateComponent>() : nullptr;
	const UGameInstance* gameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const ULRGameInstanceSubsystem* subsystem = gameInstance ? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	Tuning = subsystem && subsystem->GetTuningSet() ? subsystem->GetTuningSet()->Interaction : nullptr;
	if (!ensureMsgf(Inventory && State && Tuning, TEXT("%s requires Inventory, State, and Interaction tuning."), *GetNameSafe(this)))
	{
		return;
	}
	ScanCandidates();
	GetWorld()->GetTimerManager().SetTimer(QueryTimer, this, &ULRInteractionComponent::ScanCandidates,
		Tuning->QueryIntervalSeconds, true);
}

void ULRInteractionComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(QueryTimer);
	}
	Super::EndPlay(endPlayReason);
}

FLRInteractionResult ULRInteractionComponent::PerformPrimaryInteraction()
{
	FLRInteractionResult result;
	result.ActionTag = CurrentOption.ActionTag;
	if (!CurrentTarget.IsValid())
	{
		result.FailureReason = LRGameplayTags::InteractionRejectNoTarget;
	}
	else if (CurrentRange != ELRInteractionRange::Executable)
	{
		result.FailureReason = LRGameplayTags::InteractionRejectTooFar;
	}
	else
	{
		result = ILRInteractable::Execute_ExecuteInteraction(CurrentTarget.Get(), GetOwner(), CurrentOption.ActionTag);
	}
	OnInteractionExecuted.Broadcast(result);
	return result;
}

void ULRInteractionComponent::LogDiagnostics() const
{
	UE_LOG(LogLostRunicInteraction, Display, TEXT("Owner=%s Target=%s Action=%s Range=%d"), *GetNameSafe(GetOwner()),
		*GetNameSafe(CurrentTarget.Get()), *CurrentOption.ActionTag.ToString(), static_cast<int32>(CurrentRange));
}

void ULRInteractionComponent::ScanCandidates()
{
	if (!GetWorld() || !Inventory || !State)
	{
		return;
	}

	TArray<FCandidate> candidates;
	TArray<FLRInteractionCandidateScore> scores;
	const FVector ownerLocation = GetOwner()->GetActorLocation();
	const FVector ownerForward = GetOwner()->GetActorForwardVector().GetSafeNormal2D();
	const FGameplayTagContainer ownedTags = Inventory->GetOwnedItemTags();
	for (TActorIterator<AActor> actorIt(GetWorld()); actorIt; ++actorIt)
	{
		AActor* actor = *actorIt;
		if (!actor || actor == GetOwner() || !actor->GetClass()->ImplementsInterface(ULRInteractable::StaticClass()))
		{
			continue;
		}
		const FVector targetLocation = ILRInteractable::Execute_GetInteractionLocation(actor);
		const FVector toTarget = targetLocation - ownerLocation;
		for (const FLRInteractionOption& option : ILRInteractable::Execute_GetInteractionOptions(actor, GetOwner()))
		{
			FCandidate candidate;
			candidate.Actor = actor;
			candidate.Option = option;
			candidate.Score.Distance = toTarget.Size2D();
			candidate.Score.ForwardDot = FVector::DotProduct(ownerForward, toTarget.GetSafeNormal2D());
			candidate.Score.bOccluded = IsOccluded(actor, targetLocation);
			candidate.Score.bModeAllowed = option.RequiredMode == State->GetCurrentMode();
			candidate.Score.bItemsAllowed = option.RequiredItemTags.IsEmpty() || option.RequiredItemTags.Matches(ownedTags);
			candidate.ExecuteDistance = option.MaxDistanceOverride > 0.0f
				? option.MaxDistanceOverride : GetEffectiveTuning().ExecuteDistance;
			candidates.Add(candidate);
			scores.Add(candidate.Score);
		}
	}
	ApplySelection(candidates, LRInteractionRules::SelectBestCandidate(scores, GetEffectiveTuning()));
}

bool ULRInteractionComponent::IsOccluded(AActor* target, const FVector& targetLocation) const
{
	FHitResult hit;
	FCollisionQueryParams queryParams(SCENE_QUERY_STAT(LRInteractionOcclusion), false, GetOwner());
	const bool bHit = GetWorld()->LineTraceSingleByChannel(hit, GetOwner()->GetActorLocation(), targetLocation,
		ECC_Visibility, queryParams);
	return bHit && hit.GetActor() != target;
}

void ULRInteractionComponent::ApplySelection(const TArray<FCandidate>& candidates, const int32 selectedIndex)
{
	AActor* selectedTarget = candidates.IsValidIndex(selectedIndex) ? candidates[selectedIndex].Actor.Get() : nullptr;
	const FLRInteractionOption selectedOption = selectedTarget ? candidates[selectedIndex].Option : FLRInteractionOption();
	const ELRInteractionRange selectedRange = selectedTarget
		? LRInteractionRules::GetRange(candidates[selectedIndex].Score.Distance,
			candidates[selectedIndex].ExecuteDistance, GetEffectiveTuning()) : ELRInteractionRange::None;
	const bool bChanged = CurrentTarget.Get() != selectedTarget || CurrentOption.ActionTag != selectedOption.ActionTag
		|| CurrentRange != selectedRange;
	CurrentTarget = selectedTarget;
	CurrentOption = selectedOption;
	CurrentRange = selectedRange;
	if (bChanged)
	{
		OnTargetChanged.Broadcast(selectedTarget, CurrentOption, CurrentRange);
	}
}

const ULRInteractionTuning& ULRInteractionComponent::GetEffectiveTuning() const
{
	return Tuning ? *Tuning : *GetDefault<ULRInteractionTuning>();
}
