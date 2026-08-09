/**
 * @file LRGuardTypes.h
 * @brief 实现“家”垂直切片的守卫感知、0-11 警戒值、StateTree 行为切换、调查追逐与捕获死亡流程。规则层只计算状态，Controller 负责接入 UE 感知、导航和计时器。
 *
 * 关联文件：AI 目录内调用该公共契约的实现文件；所属领域：AI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "CoreMinimal.h"

#include "LRGuardTypes.generated.h"

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Guard Behavior"))
enum class ELRGuardBehaviorState : uint8
{
	IdlePatrol UMETA(DisplayName = "Idle / Patrol"),
	Suspicious UMETA(DisplayName = "Suspicious"),
	Investigate UMETA(DisplayName = "Investigate"),
	Search UMETA(DisplayName = "Search"),
	Chase UMETA(DisplayName = "Chase")
};
