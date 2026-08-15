/** @file LRSaveFormatting.h @brief 存档时间、时长与收藏进度的 UI 格式化纯函数。 */
#pragma once

#include "CoreMinimal.h"

namespace LRSaveFormatting
{
	/** Converts a persisted UTC timestamp to local time and formats it with the active culture. */
	LOSTRUNIC_API FText FormatSavedAtLocal(const FDateTime& savedAtUtc);

	/** Formats an injected local offset for deterministic localization tests. */
	LOSTRUNIC_API FText FormatSavedAtWithOffset(const FDateTime& savedAtUtc, FTimespan localOffset);

	/** Formats play time as an unwrapped HH:MM:SS value. */
	LOSTRUNIC_API FText FormatPlayTime(double playTimeSeconds);

	/** Formats collected/total progress without embedding localized labels. */
	LOSTRUNIC_API FText FormatCollectedCount(int32 collectedCount, int32 totalCount);
}
