/**
 * @file LRInteractionRules.h
 * @brief 实现统一交互契约：按距离、总朝向角、遮挡和当前状态筛选唯一目标，并以结构化选项和结果连接 UI、背包选物及可交互对象。
 *
 * 关联文件：LRInteractionRules.cpp；所属领域：Interaction。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Interaction/LRInteractionTypes.h"

class ULRInteractionTuning;

namespace LRInteractionRules
{
	/** Maps a visible target's squared distance to its world presentation state. */
	LOSTRUNIC_API ELRInteractionPresentationState GetPresentationState(float distanceSquared,
		const ULRInteractionTuning& tuning);
	/** Tests execution radius without introducing a square-root operation. */
	LOSTRUNIC_API bool IsWithinExecutionDistance(float distanceSquared, float executeDistance);
	LOSTRUNIC_API int32 SelectBestCandidate(const TArray<FLRInteractionCandidateScore>& candidates,
		const ULRInteractionTuning& tuning);
	/**
	 * @brief 判断 Is Facing Allowed 对应条件；不产生玩法副作用。
	 * @param forwardDot 调用方提供的 `forwardDot`，只在本次操作范围内使用。
	 * @param tuning 数据或调优来源 `tuning`；调用期间只读，并按稳定 ID 解析内容。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	LOSTRUNIC_API bool IsFacingAllowed(float forwardDot, const ULRInteractionTuning& tuning);
}
