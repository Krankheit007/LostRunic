// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @file StrategyUI.cpp
 * @brief 保留 Unreal Strategy 模板玩法，用于回归和 PIE 冒烟；它与 /Game/LostRunic 的“家”切片相互独立，不承载 LostRunic 核心叙事规则。
 *
 * 关联文件：StrategyUI.h；所属领域：Variant_Strategy。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */


#include "StrategyUI.h"

/**
 * @brief 更新 Selected Units Count，并在需要时同步组件状态或广播变化事件。
 * @param Count 本次操作使用的计数、增量或索引 `Count`；由函数校验合法范围。
 */
void UStrategyUI::SetSelectedUnitsCount(int32 Count)
{
	// is this a different count?
	bool bChanged = SelectedUnitCount != Count;

	// update the counter
	SelectedUnitCount = Count;

	// if the count changed, call the BP handler
	if (bChanged)
	{
		BP_UpdateUnitsCount();
	}
}
