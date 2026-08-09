/**
 * @file LRGuardPerceptionRules.h
 * @brief 实现“家”垂直切片的守卫感知、0-11 警戒值、StateTree 行为切换、调查追逐与捕获死亡流程。规则层只计算状态，Controller 负责接入 UE 感知、导航和计时器。
 *
 * 关联文件：LRGuardPerceptionRules.cpp；所属领域：AI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

class ULRGuardTuning;

namespace LRGuardPerceptionRules
{
	LOSTRUNIC_API bool CanConfirmSight(float distance, float forwardDot, bool bOccluded, bool bHidden,
		const ULRGuardTuning& tuning);
	/**
	 * @brief 判断 Can Hear 对应条件；不产生玩法副作用。
	 * @param distance 空间值 `distance`；距离和位置使用 Unreal 厘米单位。
	 * @param sourceRadius 空间值 `sourceRadius`；距离和位置使用 Unreal 厘米单位。
	 * @param tuning 数据或调优来源 `tuning`；调用期间只读，并按稳定 ID 解析内容。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	LOSTRUNIC_API bool CanHear(float distance, float sourceRadius, const ULRGuardTuning& tuning);
}
