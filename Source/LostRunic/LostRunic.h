// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @file LostRunic.h
 * @brief 提供 Unreal 模块入口及原始 TopDown 模板兼容类；新的“家”垂直切片使用 Framework、State、Interaction、AI、Narrative 与 Save 目录中的 LR 领域实现。
 *
 * 关联文件：LostRunic.cpp；所属领域：Root。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */

#pragma once

#include "CoreMinimal.h"

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
DECLARE_LOG_CATEGORY_EXTERN(LogLostRunic, Log, All);
