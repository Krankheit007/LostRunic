/**
 * @file LRGuardPerceptionRules.cpp
 * @brief Guard 噪声响应纯规则实现。
 */
#include "AI/LRGuardPerceptionRules.h"
#include "AI/LRAlertRules.h"

#include "Core/LRGameplayTags.h"
#include "Core/LRGuardConstants.h"
#include "Data/LRGuardTuning.h"

FLRNoiseResponse LRGuardPerceptionRules::ResolveNoiseAlertDelta(const FGameplayTag reason,
	const ELRGuardNoisePropagationMode propagationMode, const int32 currentAlert,
	const FLRGuardTuningSettings& tuning)
{
	FLRNoiseResponse response;
	if (reason == LRGameplayTags::NoiseFootstepWalkFaint
		&& currentAlert < LRAlertRules::InvestigateMinLevel)
	{
		return response;
	}

	if (reason == LRGameplayTags::NoiseFootstepRunIndoor
		&& propagationMode == ELRGuardNoisePropagationMode::CurrentRoom)
	{
		response.bRespond = true;
		response.bUseCurrentRoomRunFloor = true;
		response.Delta = tuning.AttractAlertAmount;
		return response;
	}

	if (reason == LRGameplayTags::NoiseFootstepRunIndoor
		&& propagationMode == ELRGuardNoisePropagationMode::AdjacentRoom)
	{
		response.bRespond = true;
		response.Delta = tuning.AdjacentRoomRunAlertAmount;
		return response;
	}

	response.bRespond = true;
	response.Delta = tuning.AttractAlertAmount;
	return response;
}

int32 LRGuardPerceptionRules::ResolveNoiseResultLevel(const int32 currentAlert,
	const FLRNoiseResponse& response, const FLRGuardTuningSettings& tuning)
{
	if (!response.bRespond)
	{
		return FMath::Clamp(currentAlert, LRGuardConstants::MinAlertLevel, LRAlertRules::MaxAlertLevel);
	}

	if (currentAlert >= LRAlertRules::ConfirmedAlertLevel)
	{
		return LRAlertRules::ConfirmedAlertLevel;
	}

	if (response.bUseCurrentRoomRunFloor && currentAlert < tuning.RoomRunAlertLevel)
	{
		return tuning.RoomRunAlertLevel;
	}

	return FMath::Clamp(currentAlert + response.Delta, LRGuardConstants::MinAlertLevel,
		LRAlertRules::InvestigateMaxLevel);
}
