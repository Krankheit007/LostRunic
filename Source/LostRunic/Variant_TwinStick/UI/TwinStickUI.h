// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @file TwinStickUI.h
 * @brief 保留 Unreal TwinStick 模板玩法，用于回归和 PIE 冒烟；它与 /Game/LostRunic 的“家”切片相互独立，不承载 LostRunic 核心叙事规则。
 *
 * 关联文件：TwinStickUI.cpp；所属领域：Variant_TwinStick。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TwinStickUI.generated.h"

/**
 *  A simple Twin Stick Shooter UI widget
 *  Provides a blueprint interface to expose score values to the UI
 */
UCLASS(abstract)
class UTwinStickUI : public UUserWidget
{
	GENERATED_BODY()

public:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 根据最新领域状态刷新 Update Items，并仅在值变化时通知订阅者。
	 * @param Score 调用方提供的 `Score`，只在本次操作范围内使用。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="Score")
	void UpdateItems(int32 Score);

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 根据最新领域状态刷新 Update Score，并仅在值变化时通知订阅者。
	 * @param Score 调用方提供的 `Score`，只在本次操作范围内使用。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="Score")
	void UpdateScore(int32 Score);

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 根据最新领域状态刷新 Update Combo，并仅在值变化时通知订阅者。
	 * @param Combo 调用方提供的 `Combo`，只在本次操作范围内使用。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="Score")
	void UpdateCombo(int32 Combo);
};
