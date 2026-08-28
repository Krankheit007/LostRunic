/**
 * @file LRAlertRules.h
 * @brief Guard 0-11 警戒档位、行为和噪声首次冷却的纯规则。
 */
#pragma once

#include "AI/LRGuardTypes.h"
#include "Core/LRGuardConstants.h"

struct FLRGuardTuningSettings;

namespace LRAlertRules
{
	inline constexpr int32 MinAlertLevel = LRGuardConstants::MinAlertLevel;
	inline constexpr int32 MaxAlertLevel = LRGuardConstants::MaxAlertLevel;
	inline constexpr int32 SuspiciousMinLevel = 1;
	inline constexpr int32 SuspiciousMaxLevel = 5;
	inline constexpr int32 InvestigateMinLevel = 6;
	inline constexpr int32 InvestigateMaxLevel = 10;
	inline constexpr int32 ConfirmedAlertLevel = 11;

	/** 按全局边界应用警戒变化。 */
	LOSTRUNIC_API int32 ApplyDelta(int32 currentLevel, int32 delta);

	/** 只按警戒值解析行为；Stunned 由覆盖参数优先。 */
	LOSTRUNIC_API ELRGuardBehaviorState ResolveState(int32 alertLevel);
	LOSTRUNIC_API ELRGuardBehaviorState ResolveTargetBehavior(bool bStunned, int32 alertLevel);

	/** 解析警戒条显示档位。 */
	LOSTRUNIC_API ELRGuardAlertTier ResolveAlertTier(int32 alertLevel);

	/** 取得 Knowledge 唯一执行调查点。 */
	LOSTRUNIC_API FVector ResolveInvestigationLocation(const FLRGuardKnowledgeSnapshot& knowledge);

	/** 解析一次噪声事件应使用的冷却；首次进入结果档位才使用步态倍率。 */
	LOSTRUNIC_API float ResolveAttractCooldown(int32 resultAlertLevel, bool bFirstAttractInBand,
		ELRMovementPace sourcePace, const FLRGuardTuningSettings& tuning);

	/** 判断时间冷却是否结束。 */
	LOSTRUNIC_API bool IsIncreaseAllowed(double nowSeconds, double lastIncreaseTimeSeconds, float cooldownSeconds);
}
