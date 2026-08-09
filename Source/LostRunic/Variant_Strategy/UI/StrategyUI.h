// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @file StrategyUI.h
 * @brief 保留 Unreal Strategy 模板玩法，用于回归和 PIE 冒烟；它与 /Game/LostRunic 的“家”切片相互独立，不承载 LostRunic 核心叙事规则。
 *
 * 关联文件：StrategyUI.cpp；所属领域：Variant_Strategy。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StrategyUI.generated.h"

/**
 *  Simple UI widget for the strategy game
 *	Keeps track of the number of units currently selected
 */
UCLASS(abstract)
class UStrategyUI : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Selected Unit Count 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	int32 SelectedUnitCount = 0;

public:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 更新 Selected Units Count，并在需要时同步组件状态或广播变化事件。
	 * @param Count 本次操作使用的计数、增量或索引 `Count`；由函数校验合法范围。
	 */
	void SetSelectedUnitsCount(int32 Count);

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 BP_Update Units Count 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="UI", meta = (DisplayName="Update Units Count"))
	void BP_UpdateUnitsCount();

protected:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 查询 Selected Units Count；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category="UI")
	int32 GetSelectedUnitsCount() { return SelectedUnitCount; }
};
