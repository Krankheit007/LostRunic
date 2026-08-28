/**
 * @file LRGuardPerceptionRules.h
 * @brief Guard 噪声响应的纯规则；UE Sight 的半径、角度和 LOS 由 UAISense_Sight 唯一负责。
 */
#pragma once

#include "AI/LRGuardTypes.h"
#include "GameplayTagContainer.h"

struct FLRGuardTuningSettings;

/** 噪声事件到警戒增长的语义映射。 */
struct LOSTRUNIC_API FLRNoiseResponse
{
	bool bRespond = false;
	bool bIsAttract = true;
	bool bUseCurrentRoomRunFloor = false;
	int32 Delta = 0;
};

namespace LRGuardPerceptionRules
{
	/** 解析噪声是否响应、增加量和当前房奔跑 Floor 语义。 */
	LOSTRUNIC_API FLRNoiseResponse ResolveNoiseAlertDelta(FGameplayTag reason,
		ELRGuardNoisePropagationMode propagationMode, int32 currentAlert,
		const FLRGuardTuningSettings& tuning);

	/** 将噪声响应应用到 0-10，只有视觉确认才可进入11。 */
	LOSTRUNIC_API int32 ResolveNoiseResultLevel(int32 currentAlert, const FLRNoiseResponse& response,
		const FLRGuardTuningSettings& tuning);
}
