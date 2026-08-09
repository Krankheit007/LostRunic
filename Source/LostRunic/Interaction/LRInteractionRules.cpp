#include "Interaction/LRInteractionRules.h"

#include "Data/LRInteractionTuning.h"

int32 LRInteractionRules::SelectBestCandidate(const TArray<FLRInteractionCandidateScore>& candidates,
	const ULRInteractionTuning& tuning)
{
	int32 bestIndex = INDEX_NONE;
	float bestDistance = TNumericLimits<float>::Max();
	for (int32 index = 0; index < candidates.Num(); ++index)
	{
		const FLRInteractionCandidateScore& candidate = candidates[index];
		const bool bValid = candidate.Distance <= tuning.FarHintDistance
			&& IsFacingAllowed(candidate.ForwardDot, tuning)
			&& !candidate.bOccluded && candidate.bModeAllowed && candidate.bItemsAllowed;
		if (bValid && candidate.Distance < bestDistance)
		{
			bestIndex = index;
			bestDistance = candidate.Distance;
		}
	}
	return bestIndex;
}

ELRInteractionRange LRInteractionRules::GetRange(const float distance, const float executeDistance,
	const ULRInteractionTuning& tuning)
{
	if (distance <= executeDistance)
	{
		return ELRInteractionRange::Executable;
	}
	if (distance <= tuning.OutlineDistance)
	{
		return ELRInteractionRange::Outline;
	}
	return distance <= tuning.FarHintDistance ? ELRInteractionRange::FarHint : ELRInteractionRange::None;
}

bool LRInteractionRules::IsFacingAllowed(const float forwardDot, const ULRInteractionTuning& tuning)
{
	const float halfAngleRadians = FMath::DegreesToRadians(tuning.FacingConeDegrees * 0.5f);
	return forwardDot >= FMath::Cos(halfAngleRadians);
}
