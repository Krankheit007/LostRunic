/**
 * @file LRGuardPerceptionRules.cpp
 * @brief 实现“家”垂直切片的守卫感知、0-11 警戒值、StateTree 行为切换、调查追逐与捕获死亡流程。规则层只计算状态，Controller 负责接入 UE 感知、导航和计时器。
 *
 * 关联文件：LRGuardPerceptionRules.h；所属领域：AI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "AI/LRGuardPerceptionRules.h"

#include "Core/LRGameplayTags.h"
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

/**
 * @brief 按噪声原因标签解析守卫应做的警戒响应；CD 与观察时序由调用方组件执行，本函数只做语义映射。
 * @param reason 噪声原因 Gameplay Tag，例如 Noise.Footstep.Walk 或 Noise.Footstep.Run.Indoor。
 * @param currentAlert 守卫当前警戒值 0-11。
 * @param tuning 数据或调优来源 `tuning`；调用期间只读，并按稳定 ID 解析内容。
 * @return 结构化响应：是否响应、Delta 与是否走吸引语义（IsAttract 时调用方使用带 CD 门控的 ApplyAttract）。
 */
FLRNoiseResponse LRGuardPerceptionRules::ResolveNoiseAlertDelta(const FGameplayTag reason, const int32 currentAlert,
	const ULRGuardTuning& tuning)
{
	FLRNoiseResponse response;
	if (reason == LRGameplayTags::NoiseFootstepRunIndoor)
	{
		// 室内奔跑为「警戒至少提升到 RoomRunAlertLevel」的 Set 语义，不走吸引 CD。
		response.bRespond = true;
		response.Delta = FMath::Max(tuning.RoomRunAlertLevel - currentAlert, 0);
		response.bIsAttract = false;
		return response;
	}
	if (reason == LRGameplayTags::NoiseFootstepWalkFaint)
	{
		// 室外非潜行关走路：只有警戒 >=6 的守卫才会被吸引。
		response.bIsAttract = true;
		response.bRespond = currentAlert >= tuning.SightInvestigateLevel;
		response.Delta = response.bRespond ? tuning.AttractAlertAmount : 0;
		return response;
	}
	response.bRespond = true;
	response.Delta = tuning.AttractAlertAmount;
	response.bIsAttract = true;
	return response;
}
