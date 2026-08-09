/**
 * @file LRInteractionRules.cpp
 * @brief 实现统一交互契约：按距离、总朝向角、遮挡和当前状态筛选唯一目标，并以结构化选项和结果连接 UI、背包选物及可交互对象。
 *
 * 关联文件：LRInteractionRules.h；所属领域：Interaction。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Interaction/LRInteractionRules.h"

#include "Data/LRInteractionTuning.h"

/**
 * @brief 对已通过基础检查的交互候选排序，优先选择朝向范围内距离最近者。
 * @param candidates 本次领域操作的结构化数据 `candidates`；字段语义由对应 USTRUCT 定义。
 * @param tuning 数据或调优来源 `tuning`；调用期间只读，并按稳定 ID 解析内容。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
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

/**
 * @brief 查询 Range；不修改领域状态。
 * @param distance 空间值 `distance`；距离和位置使用 Unreal 厘米单位。
 * @param executeDistance 空间值 `executeDistance`；距离和位置使用 Unreal 厘米单位。
 * @param tuning 数据或调优来源 `tuning`；调用期间只读，并按稳定 ID 解析内容。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
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

/**
 * @brief 判断 Is Facing Allowed 对应条件；不产生玩法副作用。
 * @param forwardDot 调用方提供的 `forwardDot`，只在本次操作范围内使用。
 * @param tuning 数据或调优来源 `tuning`；调用期间只读，并按稳定 ID 解析内容。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool LRInteractionRules::IsFacingAllowed(const float forwardDot, const ULRInteractionTuning& tuning)
{
	const float halfAngleRadians = FMath::DegreesToRadians(tuning.FacingConeDegrees * 0.5f);
	return forwardDot >= FMath::Cos(halfAngleRadians);
}
