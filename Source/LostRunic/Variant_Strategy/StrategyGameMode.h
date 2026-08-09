// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @file StrategyGameMode.h
 * @brief 保留 Unreal Strategy 模板玩法，用于回归和 PIE 冒烟；它与 /Game/LostRunic 的“家”切片相互独立，不承载 LostRunic 核心叙事规则。
 *
 * 关联文件：StrategyGameMode.cpp；所属领域：Variant_Strategy。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "StrategyGameMode.generated.h"

/**
 *  Simple GameMode for a top down strategy game.
 */
UCLASS(abstract)
class AStrategyGameMode : public AGameModeBase
{
	GENERATED_BODY()

};
