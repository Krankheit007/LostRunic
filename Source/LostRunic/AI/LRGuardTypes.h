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
	Chase UMETA(DisplayName = "Chase"),
	Stunned UMETA(DisplayName = "Stunned")
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Guard Alert Tier"))
enum class ELRGuardAlertTier : uint8
{
	Hidden UMETA(DisplayName = "Hidden"),
	White UMETA(DisplayName = "White"),
	Red UMETA(DisplayName = "Red"),
	Full UMETA(DisplayName = "Full")
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Alert Snapshot"))
struct LOSTRUNIC_API FLRAlertSnapshot
{
	GENERATED_BODY()

	/** Level 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `0`。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Alert")
	int32 Level = 0;

	/** Fraction 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `0.0f`。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Alert")
	float Fraction = 0.0f;

	/** Tier 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `ELRGuardAlertTier::Hidden`。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Alert")
	ELRGuardAlertTier Tier = ELRGuardAlertTier::Hidden;

	/** Behavior 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `ELRGuardBehaviorState::IdlePatrol`。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Alert")
	ELRGuardBehaviorState Behavior = ELRGuardBehaviorState::IdlePatrol;

	/** Full Alert 的开关；true 表示启用，false 表示禁用。 C++ 安全默认值为 `false`。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Alert")
	bool bFullAlert = false;
};
