// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @file EnvQueryContext_MoveGoal.cpp
 * @brief 保留 Unreal Strategy 模板玩法，用于回归和 PIE 冒烟；它与 /Game/LostRunic 的“家”切片相互独立，不承载 LostRunic 核心叙事规则。
 *
 * 关联文件：EnvQueryContext_MoveGoal.h；所属领域：Variant_Strategy。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */


#include "Variant_Strategy/EnvQueryContext_MoveGoal.h"
#include "Variant_Strategy/StrategyUnit.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"

/**
 * @brief 实现 Provide Context 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 * @param QueryInstance 调用方提供的 `QueryInstance`，只在本次操作范围内使用。
 * @param ContextData 调用方提供的 `ContextData`，只在本次操作范围内使用。
 */
void UEnvQueryContext_MoveGoal::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	// get the querying unit
	if (AStrategyUnit* QuerierActor = Cast<AStrategyUnit>(QueryInstance.Owner.Get()))
	{
		// add the last recorded danger location to the context
		UEnvQueryItemType_Point::SetContextHelper(ContextData, QuerierActor->GetMovementGoal());
	}
}
