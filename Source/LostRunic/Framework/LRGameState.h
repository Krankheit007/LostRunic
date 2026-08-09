/**
 * @file LRGameState.h
 * @brief 连接 LostRunic 的 Gameplay Framework：GameMode 管理单机世界规则，PlayerController 解释 Enhanced Input 与 UI 模式，Character 只组合能力组件，GameInstanceSubsystem 提供跨地图内容与调优配置。
 *
 * 关联文件：Framework 目录内调用该公共契约的实现文件；所属领域：Framework。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "GameFramework/GameStateBase.h"

#include "LRGameState.generated.h"

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Game State"))
class LOSTRUNIC_API ALRGameState : public AGameStateBase
{
	GENERATED_BODY()
};
