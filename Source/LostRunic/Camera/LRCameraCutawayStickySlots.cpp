#include "Camera/LRCameraCutawayComponent.h"

#include "Camera/CameraComponent.h"
#include "Camera/LRCutawayTargetComponent.h"

bool LR::Cutaway::IsStickyCandidateHigherPriority(const FStickyCandidate& left,
	const FStickyCandidate& right)
{
	if (left.bActive != right.bActive)
	{
		return left.bActive;
	}
	if (left.Amount != right.Amount)
	{
		return left.Amount > right.Amount;
	}
	if (left.DistanceSquared != right.DistanceSquared)
	{
		return left.DistanceSquared < right.DistanceSquared;
	}
	return left.ObjectKey < right.ObjectKey;
}

void ULRCameraCutawayComponent::UpdateStickySlots()
{
	TSet<ULRCutawayTargetComponent*> representedTargets;
	for (int32 slotIndex = 0; slotIndex < StickySlotTargets.Num();)
	{
		ULRCutawayTargetComponent* target = StickySlotTargets[slotIndex];
		if (!IsValid(target))
		{
			if (slotIndex == StickySlotTargets.Num() - 1) break;
			ReleaseStickySlot(slotIndex);
			continue;
		}
		if (!RequestedTargets.Contains(target)
			&& target->GetCurrentCutawayAmount(ELRCutawayRequestType::Local) <= StickyReleaseThreshold)
		{
			ReleaseStickySlot(slotIndex);
			continue;
		}
		representedTargets.Add(target);
		StickySlotAmounts[slotIndex] = target->GetCurrentCutawayAmount(ELRCutawayRequestType::Local);
		UpdatingTargets.AddUnique(target);
		++slotIndex;
	}

	TSet<ULRCutawayTargetComponent*> candidateTargets;
	for (ULRCutawayTargetComponent* target : UpdatingTargets)
	{
		if (IsValid(target)) candidateTargets.Add(target);
	}
	for (const TWeakObjectPtr<ULRCutawayTargetComponent>& target : RequestedTargets)
	{
		if (target.IsValid()) candidateTargets.Add(target.Get());
	}

	struct FWaitingCandidate
	{
		ULRCutawayTargetComponent* Target = nullptr;
		LR::Cutaway::FStickyCandidate Priority;
	};
	TArray<FWaitingCandidate> waitingCandidates;
	for (ULRCutawayTargetComponent* target : candidateTargets)
	{
		if (!IsValid(target) || representedTargets.Contains(target)) continue;
		const float currentAmount = target->GetCurrentCutawayAmount(ELRCutawayRequestType::Local);
		const bool bActive = RequestedTargets.Contains(target);
		if (!bActive && currentAmount <= StickyReleaseThreshold) continue;
		const FVector targetLocation = target->GetOwner() ? target->GetOwner()->GetActorLocation() : FVector::ZeroVector;
		const float distanceSquared = Camera
			? FVector::DistSquared(Camera->GetComponentLocation(), targetLocation)
			: TNumericLimits<float>::Max();
		FWaitingCandidate& candidate = waitingCandidates.AddDefaulted_GetRef();
		candidate.Target = target;
		candidate.Priority.bActive = bActive;
		candidate.Priority.Amount = currentAmount;
		candidate.Priority.DistanceSquared = distanceSquared;
		candidate.Priority.ObjectKey = FObjectKey(target);
	}
	waitingCandidates.Sort([](const FWaitingCandidate& left, const FWaitingCandidate& right)
	{
		return LR::Cutaway::IsStickyCandidateHigherPriority(left.Priority, right.Priority);
	});

	for (const FWaitingCandidate& candidate : waitingCandidates)
	{
		int32 emptySlot = INDEX_NONE;
		for (int32 slotIndex = 0; slotIndex < StickySlotTargets.Num(); ++slotIndex)
		{
			if (!IsValid(StickySlotTargets[slotIndex]))
			{
				emptySlot = slotIndex;
				break;
			}
		}
		if (emptySlot == INDEX_NONE) break;
		StickySlotTargets[emptySlot] = candidate.Target;
		StickySlotCenters[emptySlot] = FVector2D::ZeroVector;
		StickySlotRadii[emptySlot] = RadiusRefPx;
		StickySlotAmounts[emptySlot] = candidate.Target->GetCurrentCutawayAmount(ELRCutawayRequestType::Local);
		UpdatingTargets.AddUnique(candidate.Target);
	}
}

void ULRCameraCutawayComponent::RefreshStickySlotAmounts()
{
	for (int32 slotIndex = 0; slotIndex < StickySlotTargets.Num();)
	{
		ULRCutawayTargetComponent* target = StickySlotTargets[slotIndex];
		if (!IsValid(target))
		{
			if (slotIndex == StickySlotTargets.Num() - 1) break;
			ReleaseStickySlot(slotIndex);
			continue;
		}
		if (!RequestedTargets.Contains(target)
			&& target->GetCurrentCutawayAmount(ELRCutawayRequestType::Local) <= StickyReleaseThreshold)
		{
			ReleaseStickySlot(slotIndex);
			continue;
		}
		StickySlotAmounts[slotIndex] = target->GetCurrentCutawayAmount(ELRCutawayRequestType::Local);
		UpdatingTargets.AddUnique(target);
		++slotIndex;
	}
}

void ULRCameraCutawayComponent::ReleaseStickySlot(const int32 slotIndex)
{
	if (!StickySlotTargets.IsValidIndex(slotIndex)) return;
	for (int32 index = slotIndex; index < StickySlotTargets.Num() - 1; ++index)
	{
		StickySlotTargets[index] = StickySlotTargets[index + 1];
		StickySlotCenters[index] = StickySlotCenters[index + 1];
		StickySlotRadii[index] = StickySlotRadii[index + 1];
		StickySlotAmounts[index] = StickySlotAmounts[index + 1];
	}
	const int32 lastIndex = StickySlotTargets.Num() - 1;
	StickySlotTargets[lastIndex] = nullptr;
	StickySlotCenters[lastIndex] = FVector2D::ZeroVector;
	StickySlotRadii[lastIndex] = 0.0f;
	StickySlotAmounts[lastIndex] = 0.0f;
}
