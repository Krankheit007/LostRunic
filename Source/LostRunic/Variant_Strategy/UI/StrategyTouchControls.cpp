// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @file StrategyTouchControls.cpp
 * @brief 保留 Unreal Strategy 模板玩法，用于回归和 PIE 冒烟；它与 /Game/LostRunic 的“家”切片相互独立，不承载 LostRunic 核心叙事规则。
 *
 * 关联文件：StrategyTouchControls.h；所属领域：Variant_Strategy。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */


#include "Variant_Strategy/UI/StrategyTouchControls.h"
#include "Variant_Strategy/StrategyPlayerController.h"

/**
 * @brief 更新 Player Controller，并在需要时同步组件状态或广播变化事件。
 * @param PC 调用方提供的 `PC`，只在本次操作范围内使用。
 */
void UStrategyTouchControls::SetPlayerController(AStrategyPlayerController* PC)
{
	PlayerController = PC;
}

/**
 * @brief 实现 Reset Zoom 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 */
void UStrategyTouchControls::ResetZoom()
{
	if (PlayerController)
	{
		PlayerController->DoCameraResetZoomCommand();

		BP_SetZoomPercentage(PlayerController->GetDefaultZoomPercentage());
	}
}

/**
 * @brief 实现 Toggle Select All Units 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 */
void UStrategyTouchControls::ToggleSelectAllUnits()
{
	if (PlayerController)
	{
		PlayerController->DoToggleSelectAllUnitsCommand();
	}
}

/**
 * @brief 更新 Zoom Percentage，并在需要时同步组件状态或广播变化事件。
 * @param Percentage 调用方提供的 `Percentage`，只在本次操作范围内使用。
 */
void UStrategyTouchControls::SetZoomPercentage(float Percentage)
{
	if (PlayerController)
	{
		PlayerController->DoCameraSetZoomPercentageCommand(Percentage);
	}
}
