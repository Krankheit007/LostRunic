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
