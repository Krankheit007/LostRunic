// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @file StrategyTouchControls.h
 * @brief 保留 Unreal Strategy 模板玩法，用于回归和 PIE 冒烟；它与 /Game/LostRunic 的“家”切片相互独立，不承载 LostRunic 核心叙事规则。
 *
 * 关联文件：StrategyTouchControls.cpp；所属领域：Variant_Strategy。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StrategyTouchControls.generated.h"

class AStrategyPlayerController;

/**
 *  Base class for additional touchscreen controls for a strategy game.
 *  Exposes some game commands to UI
 */
UCLASS(abstract)
class LOSTRUNIC_API UStrategyTouchControls : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Player Controller 的内部运行时数据；不参与蓝图配置。 */
	TObjectPtr<AStrategyPlayerController> PlayerController;

public:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 更新 Player Controller，并在需要时同步组件状态或广播变化事件。
	 * @param PC 调用方提供的 `PC`，只在本次操作范围内使用。
	 */
	void SetPlayerController(AStrategyPlayerController* PC);

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 BP_Set Zoom Percentage 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 * @param Percentage 调用方提供的 `Percentage`，只在本次操作范围内使用。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="UI", meta=(DisplayName="Set Zoom Percentage"))
	void BP_SetZoomPercentage(float Percentage);

protected:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Reset Zoom 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 */
	UFUNCTION(BlueprintCallable, Category="UI")
	void ResetZoom();

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Toggle Select All Units 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 */
	UFUNCTION(BlueprintCallable, Category="UI")
	void ToggleSelectAllUnits();

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 更新 Zoom Percentage，并在需要时同步组件状态或广播变化事件。
	 * @param Percentage 调用方提供的 `Percentage`，只在本次操作范围内使用。
	 */
	UFUNCTION(BlueprintCallable, Category="UI")
	void SetZoomPercentage(float Percentage);
};
