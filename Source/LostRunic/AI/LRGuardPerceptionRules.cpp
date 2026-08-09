/**
 * @file LRGuardPerceptionRules.cpp
 * @brief 实现“家”垂直切片的守卫感知、0-11 警戒值、StateTree 行为切换、调查追逐与捕获死亡流程。规则层只计算状态，Controller 负责接入 UE 感知、导航和计时器。
 *
 * 关联文件：LRGuardPerceptionRules.h；所属领域：AI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "AI/LRGuardPerceptionRules.h"

#include "Data/LRGuardTuning.h"

/**
 * @brief 判断 Can Confirm Sight 对应条件；不产生玩法副作用。
 * @param distance 空间值 `distance`；距离和位置使用 Unreal 厘米单位。
 * @param forwardDot 调用方提供的 `forwardDot`，只在本次操作范围内使用。
 * @param bOccluded 布尔开关 `bOccluded`；true 表示启用或条件成立，false 表示禁用或条件不成立。
 * @param bHidden 布尔开关 `bHidden`；true 表示启用或条件成立，false 表示禁用或条件不成立。
 * @param tuning 数据或调优来源 `tuning`；调用期间只读，并按稳定 ID 解析内容。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool LRGuardPerceptionRules::CanConfirmSight(const float distance, const float forwardDot, const bool bOccluded,
	const bool bHidden, const ULRGuardTuning& tuning)
{
	const float halfAngleRadians = FMath::DegreesToRadians(tuning.SightConeDegrees * 0.5f);
	return distance <= tuning.SightRadius && forwardDot >= FMath::Cos(halfAngleRadians) && !bOccluded && !bHidden;
}

/**
 * @brief 判断 Can Hear 对应条件；不产生玩法副作用。
 * @param distance 空间值 `distance`；距离和位置使用 Unreal 厘米单位。
 * @param sourceRadius 空间值 `sourceRadius`；距离和位置使用 Unreal 厘米单位。
 * @param tuning 数据或调优来源 `tuning`；调用期间只读，并按稳定 ID 解析内容。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool LRGuardPerceptionRules::CanHear(const float distance, const float sourceRadius, const ULRGuardTuning& tuning)
{
	return distance <= sourceRadius * tuning.HearingRangeMultiplier;
}
