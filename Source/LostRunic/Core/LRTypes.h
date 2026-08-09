/**
 * @file LRTypes.h
 * @brief 声明 LostRunic 各玩法领域共享的稳定 ID、Gameplay Tags、日志分类、数据校验与调试命令，供状态、交互、AI、叙事和存档系统统一使用。
 *
 * 关联文件：Core 目录内调用该公共契约的实现文件；所属领域：Core。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "CoreMinimal.h"

#include "LRTypes.generated.h"

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Perception Mode"))
enum class ELRPerceptionMode : uint8
{
	Normal UMETA(DisplayName = "Normal"),
	Perception UMETA(DisplayName = "Perception"),
	Courage UMETA(DisplayName = "Courage"),
	Memory UMETA(DisplayName = "Memory")
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Input Mode"))
enum class ELRInputMode : uint8
{
	Gameplay UMETA(DisplayName = "Gameplay"),
	Dialogue UMETA(DisplayName = "Dialogue"),
	Menu UMETA(DisplayName = "Menu"),
	Transition UMETA(DisplayName = "Transition")
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Movement Pace"))
enum class ELRMovementPace : uint8
{
	Sneak UMETA(DisplayName = "Sneak"),
	Walk UMETA(DisplayName = "Walk"),
	Run UMETA(DisplayName = "Run")
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Noise Environment"))
enum class ELRNoiseEnvironment : uint8
{
	Indoor UMETA(DisplayName = "Indoor"),
	Outdoor UMETA(DisplayName = "Outdoor")
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Save Policy"))
enum class ELRSavePolicy : uint8
{
	None UMETA(DisplayName = "None"),
	AutoOnComplete UMETA(DisplayName = "Auto On Complete"),
	Critical UMETA(DisplayName = "Critical Ordered Save")
};
