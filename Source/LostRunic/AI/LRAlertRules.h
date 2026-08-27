/**
 * @file LRAlertRules.h
 * @brief 提供 0-11 警戒纯规则：边界钳制、行为档位解析（4.2.1 语义）、衰减门控与吸引增加冷却，供运行时组件与 LostRunic.AI 自动化测试共同调用。
 *
 * 关联文件：LRAlertRules.cpp；所属领域：AI。
 * 设计依据：Docs/Technical/08_ArchitectureBoundaries.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "AI/LRGuardTypes.h"
#include "Core/LRGuardConstants.h"

struct FLRGuardTuningSettings;

namespace LRAlertRules
{
	/** 警戒上限；供 UI 快照与运行时组件共享。 */
	inline constexpr int32 MaxAlertLevel = LRGuardConstants::MaxAlertLevel;
	/** 警戒下限。 */
	inline constexpr int32 MinAlertLevel = LRGuardConstants::MinAlertLevel;

	/**
	 * @brief 按 0-11 边界应用警戒变化，并返回旧值、新值和原因。
	 * @param currentLevel 本次操作使用的计数、增量或索引 `currentLevel`；由函数校验合法范围。
	 * @param delta 调用方提供的 `delta`，只在本次操作范围内使用。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	LOSTRUNIC_API int32 ApplyDelta(int32 currentLevel, int32 delta);
	/**
	 * @brief 按默认 Guard Tuning 的 Detection floors 解析行为；保留此重载作为无快照兼容入口。
	 * @param alertLevel 本次操作使用的计数、增量或索引 `alertLevel`；由函数校验合法范围。
	 * @param bHasSight 布尔开关 `bHasSight`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 * @param bSearching 布尔开关 `bSearching`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	LOSTRUNIC_API ELRGuardBehaviorState ResolveState(int32 alertLevel, bool bHasSight, bool bSearching);
	/**
	 * @brief 守卫行为唯一权威解析：眩晕覆盖优先返回 Stunned，否则按警戒推导；StateTree 只执行本结果，不自行重新定义警戒语义。
	 * @param bStunned 布尔开关 `bStunned`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 * @param alertLevel 本次操作使用的计数、增量或索引 `alertLevel`；由函数校验合法范围。
	 * @param bHasSight 布尔开关 `bHasSight`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 * @param bSearching 布尔开关 `bSearching`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	LOSTRUNIC_API ELRGuardBehaviorState ResolveTargetBehavior(bool bStunned, int32 alertLevel, bool bHasSight,
		bool bSearching);
	/** Snapshot-aware behavior resolver; Alert alone never promotes to Chase. */
	LOSTRUNIC_API ELRGuardBehaviorState ResolveTargetBehavior(bool bStunned, const FLRAlertSnapshot& alert,
		const FLRGuardKnowledgeSnapshot& knowledge, bool bSearchFlag, const FLRGuardTuningSettings& tuning);
	LOSTRUNIC_API ELRGuardBehaviorState ResolveTargetBehavior(const FLRAlertSnapshot& alert,
		const FLRGuardKnowledgeSnapshot& knowledge, bool bStunned, bool bSearchFlag,
		const FLRGuardTuningSettings& tuning);
	LOSTRUNIC_API ELRGuardBehaviorState ResolveTargetBehavior(const FLRAlertSnapshot& alert,
		const FLRGuardKnowledgeSnapshot& knowledge, bool bStunned, bool bSearchFlag);
	LOSTRUNIC_API ELRGuardBehaviorState ResolveTargetBehavior(const FLRGuardAwarenessSnapshot& awareness,
		bool bStunned, bool bSearchFlag, const FLRGuardTuningSettings& tuning);
	/**
	 * @brief 解析警戒显示档位：0 隐藏、1-5 白色、6-10 红色、11 满值。
	 * @param alertLevel 本次操作使用的计数、增量或索引 `alertLevel`；由函数校验合法范围。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	LOSTRUNIC_API ELRGuardAlertTier ResolveAlertTier(int32 alertLevel);
	/**
	 * @brief 判断 Should Decay 对应条件；观察中、追逐中或调查（前往）中不衰减，其余 0.5s/-1。
	 * @param bObserving 布尔开关 `bObserving`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 * @param bHasConfirmedSight 布尔开关 `bHasConfirmedSight`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 * @param currentState 本次操作使用的 `currentState` 枚举或模式值。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	LOSTRUNIC_API bool ShouldDecay(bool bObserving, bool bHasConfirmedSight, ELRGuardBehaviorState currentState);
	/** Snapshot-aware decay gate; active visual exposure/current visibility blocks decay. */
	LOSTRUNIC_API bool ShouldDecay(const FLRAlertSnapshot& alert, const FLRGuardKnowledgeSnapshot& knowledge,
		bool bObserving, ELRGuardBehaviorState resolvedBehavior);
	/** Resolves pending threat first, then disturbance, then the last-known threat location. */
	LOSTRUNIC_API FVector ResolveInvestigationLocation(const FLRGuardKnowledgeSnapshot& knowledge);
	LOSTRUNIC_API FVector ResolveInvestigationLocation(const FLRGuardAwarenessSnapshot& awareness);
	/**
	 * @brief 解析吸引增加的冷却时长：1-5 档与首次进入 6-10 档使用 AlertIncreaseCooldownSeconds，6-10 档后续使用 InvestigateIncreaseCooldownSeconds。
	 * @param currentAlert 本次操作使用的计数、增量或索引 `currentAlert`；由函数校验合法范围。
	 * @param bFirstIncreaseInBand 布尔开关 `bFirstIncreaseInBand`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 * @param tuning 数据或调优来源 `tuning`；调用期间只读，并按稳定 ID 解析内容。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	LOSTRUNIC_API float ResolveAttractIncreaseCooldown(int32 currentAlert, bool bFirstIncreaseInBand,
		const FLRGuardTuningSettings& tuning);
	/**
	 * @brief 判断 Is Increase Allowed 对应条件；冷却拒绝的刺激被完全忽略，不改变观察状态。
	 * @param now 时间值 `now`，单位为秒。
	 * @param lastIncreaseTime 时间值 `lastIncreaseTime`，单位为秒。
	 * @param cooldownSeconds 时间值 `cooldownSeconds`，单位为秒。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	LOSTRUNIC_API bool IsIncreaseAllowed(double now, double lastIncreaseTime, float cooldownSeconds);
}
