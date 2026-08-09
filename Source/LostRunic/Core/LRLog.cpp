/**
 * @file LRLog.cpp
 * @brief 声明 LostRunic 各玩法领域共享的稳定 ID、Gameplay Tags、日志分类、数据校验与调试命令，供状态、交互、AI、叙事和存档系统统一使用。
 *
 * 关联文件：LRLog.h；所属领域：Core。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Core/LRLog.h"

DEFINE_LOG_CATEGORY(LogLostRunicState);
DEFINE_LOG_CATEGORY(LogLostRunicInteraction);
DEFINE_LOG_CATEGORY(LogLostRunicAI);
DEFINE_LOG_CATEGORY(LogLostRunicNarrative);
DEFINE_LOG_CATEGORY(LogLostRunicSave);
DEFINE_LOG_CATEGORY(LogLostRunicUI);
DEFINE_LOG_CATEGORY(LogLostRunicTuning);
