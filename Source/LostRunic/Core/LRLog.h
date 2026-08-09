/**
 * @file LRLog.h
 * @brief 声明 LostRunic 各玩法领域共享的稳定 ID、Gameplay Tags、日志分类、数据校验与调试命令，供状态、交互、AI、叙事和存档系统统一使用。
 *
 * 关联文件：LRLog.cpp；所属领域：Core。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogLostRunicState, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogLostRunicInteraction, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogLostRunicAI, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogLostRunicNarrative, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogLostRunicSave, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogLostRunicUI, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogLostRunicTuning, Log, All);
