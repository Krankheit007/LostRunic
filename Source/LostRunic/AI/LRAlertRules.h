/**
 * @file LRAlertRules.h
 * @brief 实现“家”垂直切片的守卫感知、0-11 警戒值、StateTree 行为切换、调查追逐与捕获死亡流程。规则层只计算状态，Controller 负责接入 UE 感知、导航和计时器。
 *
 * 关联文件：LRAlertRules.cpp；所属领域：AI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "AI/LRGuardTypes.h"

namespace LRAlertRules
{
	/**
	 * @brief 按 0-11 边界应用警戒变化，并返回旧值、新值和原因。
	 * @param currentLevel 本次操作使用的计数、增量或索引 `currentLevel`；由函数校验合法范围。
	 * @param delta 调用方提供的 `delta`，只在本次操作范围内使用。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	LOSTRUNIC_API int32 ApplyDelta(int32 currentLevel, int32 delta);
	/**
	 * @brief 执行 Resolve State 的纯规则或事务判定，失败时提供结构化原因。
	 * @param alertLevel 本次操作使用的计数、增量或索引 `alertLevel`；由函数校验合法范围。
	 * @param bHasSight 布尔开关 `bHasSight`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 * @param bSearching 布尔开关 `bSearching`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	LOSTRUNIC_API ELRGuardBehaviorState ResolveState(int32 alertLevel, bool bHasSight, bool bSearching);
	/**
	 * @brief 判断 Should Decay 对应条件；不产生玩法副作用。
	 * @param secondsSinceStimulus 时间值 `secondsSinceStimulus`，单位为秒。
	 * @param observeSeconds 时间值 `observeSeconds`，单位为秒。
	 * @param bHasSight 布尔开关 `bHasSight`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	LOSTRUNIC_API bool ShouldDecay(float secondsSinceStimulus, float observeSeconds, bool bHasSight);
}
