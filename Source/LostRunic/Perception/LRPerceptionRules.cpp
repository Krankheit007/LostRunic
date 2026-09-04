/**
 * @file LRPerceptionRules.cpp
 * @brief Implements pure Perception timing and fixed-slot selection rules.
 */
#include "Perception/LRPerceptionRules.h"

namespace LRPerceptionRules
{
	bool CanRefreshSlot(const FLRPerceptionEchoSlot& slot, const FLRPerceptionPulseRequest& request,
		const float now, const float mergeDistanceCm)
	{
		return request.bRefreshExistingSource
			&& slot.bActive
			&& slot.ExpireTime > now
			&& slot.SourceObject == request.SourceObject
			&& FVector::DistSquared(slot.Center, request.WorldLocation)
			<= FMath::Square(FMath::Max(mergeDistanceCm, 0.0f));
	}

	float ComputeResidueFade(const float remainingSeconds, const float dryFadeDurationSeconds)
	{
		if (remainingSeconds <= 0.0f || dryFadeDurationSeconds <= 0.0f)
		{
			return 0.0f;
		}
		if (remainingSeconds >= dryFadeDurationSeconds)
		{
			return 1.0f;
		}
		return FMath::Clamp(remainingSeconds / dryFadeDurationSeconds, 0.0f, 1.0f);
	}

	int32 SelectSlot(const TArray<FLRPerceptionEchoSlot>& slots, const float now)
	{
		int32 expiredIndex = INDEX_NONE;
		int32 earliestIndex = INDEX_NONE;
		float earliestExpire = TNumericLimits<float>::Max();
		for (int32 index = 0; index < slots.Num(); ++index)
		{
			const FLRPerceptionEchoSlot& slot = slots[index];
			if (!slot.bActive)
			{
				return index;
			}
			if (slot.ExpireTime <= now && expiredIndex == INDEX_NONE)
			{
				expiredIndex = index;
			}
			if (slot.ExpireTime < earliestExpire)
			{
				earliestExpire = slot.ExpireTime;
				earliestIndex = index;
			}
		}
		return expiredIndex != INDEX_NONE ? expiredIndex : earliestIndex;
	}
}
