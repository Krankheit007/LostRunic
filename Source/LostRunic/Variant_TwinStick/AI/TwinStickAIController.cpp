// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @file TwinStickAIController.cpp
 * @brief 保留 Unreal TwinStick 模板玩法，用于回归和 PIE 冒烟；它与 /Game/LostRunic 的“家”切片相互独立，不承载 LostRunic 核心叙事规则。
 *
 * 关联文件：TwinStickAIController.h；所属领域：Variant_TwinStick。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */


#include "TwinStickAIController.h"
#include "Components/StateTreeAIComponent.h"

/**
 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
 */
ATwinStickAIController::ATwinStickAIController()
{
	// create the StateTree AI Component
	StateTreeAI = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAI"));
	check(StateTreeAI);

	// ensure we start the StateTree when we possess the pawn
	bStartAILogicOnPossess = true;

	// ensure we're attached to the possessed character.
	// this is necessary for EnvQueries to work correctly
	bAttachToPawn = true;
}
