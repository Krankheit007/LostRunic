/**
 * @file LRValidation.h
 * @brief 声明 LostRunic 各玩法领域共享的稳定 ID、Gameplay Tags、日志分类、数据校验与调试命令，供状态、交互、AI、叙事和存档系统统一使用。
 *
 * 关联文件：Core 目录内调用该公共契约的实现文件；所属领域：Core。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "CoreMinimal.h"

namespace LRValidation
{
	inline bool RequirePositive(const TCHAR* fieldName, const float value, FString& outError)
	{
		if (value > 0.0f)
		{
			return true;
		}

		outError = FString::Printf(TEXT("%s must be greater than zero; actual %.3f."), fieldName, value);
		return false;
	}

	inline bool RequireRange(const TCHAR* fieldName, const float value, const float minValue, const float maxValue, FString& outError)
	{
		if (value >= minValue && value <= maxValue)
		{
			return true;
		}

		outError = FString::Printf(TEXT("%s must be in [%.3f, %.3f]; actual %.3f."), fieldName, minValue, maxValue, value);
		return false;
	}
}
