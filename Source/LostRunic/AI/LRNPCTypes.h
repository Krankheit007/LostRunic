/**
 * @file LRNPCTypes.h
 * @brief 声明通用 NPC 的行为状态与定义配置枚举，供 NPC 角色、控制器、StateTree 节点与内容定义共享。
 *
 * 关联文件：AI 目录内调用该公共契约的实现文件；所属领域：AI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "CoreMinimal.h"

#include "LRNPCTypes.generated.h"

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic NPC Behavior"))
enum class ELRNPCBehaviorState : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Patrol UMETA(DisplayName = "Patrol"),
	ReactToNoise UMETA(DisplayName = "React To Noise"),
	Conversation UMETA(DisplayName = "Conversation")
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic NPC Base Behavior"))
enum class ENPCBaseBehavior : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Patrol UMETA(DisplayName = "Patrol")
};
