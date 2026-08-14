/**
 * @file LRAlertRules.cpp
 * @brief 实现 0-11 警戒纯规则：边界钳制、行为档位解析（4.2.1）、衰减门控与吸引增加冷却。
 *
 * 关联文件：LRAlertRules.h；所属领域：AI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "AI/LRAlertRules.h"

#include "Data/LRGuardTuning.h"

namespace
{
	constexpr int32 SuspiciousMaxLevel = 5;
}

/**
 * @brief 按 0-11 边界应用警戒变化，并返回旧值、新值和原因。
 * @param currentLevel 本次操作使用的计数、增量或索引 `currentLevel`；由函数校验合法范围。
 * @param delta 调用方提供的 `delta`，只在本次操作范围内使用。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
int32 LRAlertRules::ApplyDelta(const int32 currentLevel, const int32 delta)
{
	return FMath::Clamp(currentLevel + delta, MinAlertLevel, MaxAlertLevel);
}

/**
 * @brief 按 4.2.1 档位解析行为：0 巡逻；11+视线 追逐；11 无视线 搜索兜底；搜索且 >=6 搜索；<=5 可疑；否则调查。
 * @param alertLevel 本次操作使用的计数、增量或索引 `alertLevel`；由函数校验合法范围。
 * @param bHasSight 布尔开关 `bHasSight`；true 表示启用或条件成立，false 表示禁用或条件不成立。
 * @param bSearching 布尔开关 `bSearching`；true 表示启用或条件成立，false 表示禁用或条件不成立。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
ELRGuardBehaviorState LRAlertRules::ResolveState(const int32 alertLevel, const bool bHasSight, const bool bSearching)
{
	if (alertLevel <= MinAlertLevel)
	{
		return ELRGuardBehaviorState::IdlePatrol;
	}
	if (bHasSight && alertLevel >= MaxAlertLevel)
	{
		return ELRGuardBehaviorState::Chase;
	}
	if (alertLevel >= MaxAlertLevel)
	{
		return ELRGuardBehaviorState::Search;
	}
	if (bSearching && alertLevel >= SuspiciousMaxLevel + 1)
	{
		return ELRGuardBehaviorState::Search;
	}
	return alertLevel <= SuspiciousMaxLevel
		? ELRGuardBehaviorState::Suspicious : ELRGuardBehaviorState::Investigate;
}

/**
 * @brief 守卫行为唯一权威解析：眩晕覆盖优先返回 Stunned，否则按警戒推导；StateTree 只执行本结果，不自行重新定义警戒语义。
 * @param bStunned 布尔开关 `bStunned`；true 表示启用或条件成立，false 表示禁用或条件不成立。
 * @param alertLevel 本次操作使用的计数、增量或索引 `alertLevel`；由函数校验合法范围。
 * @param bHasSight 布尔开关 `bHasSight`；true 表示启用或条件成立，false 表示禁用或条件不成立。
 * @param bSearching 布尔开关 `bSearching`；true 表示启用或条件成立，false 表示禁用或条件不成立。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
ELRGuardBehaviorState LRAlertRules::ResolveTargetBehavior(const bool bStunned, const int32 alertLevel,
	const bool bHasSight, const bool bSearching)
{
	return bStunned ? ELRGuardBehaviorState::Stunned : ResolveState(alertLevel, bHasSight, bSearching);
}

/**
 * @brief 解析警戒显示档位：0 隐藏、1-5 白色、6-10 红色、11 满值。
 * @param alertLevel 本次操作使用的计数、增量或索引 `alertLevel`；由函数校验合法范围。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
ELRGuardAlertTier LRAlertRules::ResolveAlertTier(const int32 alertLevel)
{
	if (alertLevel <= MinAlertLevel)
	{
		return ELRGuardAlertTier::Hidden;
	}
	if (alertLevel >= MaxAlertLevel)
	{
		return ELRGuardAlertTier::Full;
	}
	return alertLevel <= SuspiciousMaxLevel ? ELRGuardAlertTier::White : ELRGuardAlertTier::Red;
}

/**
 * @brief 判断 Should Decay 对应条件；观察中、追逐中或调查（前往）中不衰减，其余 0.5s/-1。
 * @param bObserving 布尔开关 `bObserving`；true 表示启用或条件成立，false 表示禁用或条件不成立。
 * @param bHasConfirmedSight 布尔开关 `bHasConfirmedSight`；true 表示启用或条件成立，false 表示禁用或条件不成立。
 * @param currentState 本次操作使用的 `currentState` 枚举或模式值。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool LRAlertRules::ShouldDecay(const bool bObserving, const bool bHasConfirmedSight,
	const ELRGuardBehaviorState currentState)
{
	if (bObserving || bHasConfirmedSight)
	{
		return false;
	}
	return currentState != ELRGuardBehaviorState::Investigate && currentState != ELRGuardBehaviorState::Chase;
}

/**
 * @brief 解析吸引增加的冷却时长：1-5 档与首次进入 6-10 档使用 AlertIncreaseCooldownSeconds，6-10 档后续使用 InvestigateIncreaseCooldownSeconds。
 * @param currentAlert 本次操作使用的计数、增量或索引 `currentAlert`；由函数校验合法范围。
 * @param bFirstIncreaseInBand 布尔开关 `bFirstIncreaseInBand`；true 表示启用或条件成立，false 表示禁用或条件不成立。
 * @param tuning 数据或调优来源 `tuning`；调用期间只读，并按稳定 ID 解析内容。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
float LRAlertRules::ResolveAttractIncreaseCooldown(const int32 currentAlert, const bool bFirstIncreaseInBand,
	const ULRGuardTuning& tuning)
{
	if (currentAlert < tuning.SightInvestigateLevel || bFirstIncreaseInBand)
	{
		return tuning.AlertIncreaseCooldownSeconds;
	}
	return tuning.InvestigateIncreaseCooldownSeconds;
}

/**
 * @brief 判断 Is Increase Allowed 对应条件；冷却拒绝的刺激被完全忽略，不改变观察状态。
 * @param now 时间值 `now`，单位为秒。
 * @param lastIncreaseTime 时间值 `lastIncreaseTime`，单位为秒。
 * @param cooldownSeconds 时间值 `cooldownSeconds`，单位为秒。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool LRAlertRules::IsIncreaseAllowed(const double now, const double lastIncreaseTime, const float cooldownSeconds)
{
	return cooldownSeconds <= 0.0f || now - lastIncreaseTime >= cooldownSeconds;
}
