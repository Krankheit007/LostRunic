// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @file StrategyHUD.h
 * @brief 保留 Unreal Strategy 模板玩法，用于回归和 PIE 冒烟；它与 /Game/LostRunic 的“家”切片相互独立，不承载 LostRunic 核心叙事规则。
 *
 * 关联文件：StrategyHUD.cpp；所属领域：Variant_Strategy。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "StrategyHUD.generated.h"

class UStrategyUI;

/**
 *  Simple strategy game HUD
 *  Draws the selection box and unit selected overlays
 */
UCLASS(abstract)
class AStrategyHUD : public AHUD
{
	GENERATED_BODY()

protected:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** UIWidget 的领域数据，由所属类型负责维护和校验。  */
	UPROPERTY()
	TObjectPtr<UStrategyUI> UIWidget;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** UIWidget Class 的软类或类默认引用，用于创建对应蓝图实例。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, Category="UI")
	TSubclassOf<UStrategyUI> UIWidgetClass;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Draw Box 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	bool bDrawBox = false;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Box Start 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	FVector2D BoxStart;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Box Size 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	FVector2D BoxSize;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Box Current Position 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	FVector2D BoxCurrentPosition;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Selection Box Color 的领域数据，由所属类型负责维护和校验。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, Category="UI")
	FLinearColor SelectionBoxColor;

public:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 在进入世界后解析运行时依赖、绑定事件并启动所需计时器；构造阶段不访问 World 或玩家对象。
	 */
	virtual void BeginPlay() override;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Drag Select Update 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 * @param Start 调用方提供的 `Start`，只在本次操作范围内使用。
	 * @param WidthAndHeight 调用方提供的 `WidthAndHeight`，只在本次操作范围内使用。
	 * @param CurrentPosition 调用方提供的 `CurrentPosition`，只在本次操作范围内使用。
	 * @param bDraw 布尔开关 `bDraw`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 */
	void DragSelectUpdate(FVector2D Start, FVector2D WidthAndHeight, FVector2D CurrentPosition, bool bDraw);

protected:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Draw HUD 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 */
	virtual void DrawHUD() override;
};
