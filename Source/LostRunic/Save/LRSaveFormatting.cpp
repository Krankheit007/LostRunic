#include "Save/LRSaveFormatting.h"

#include "Internationalization/Text.h"

namespace LRSaveFormatting
{
	FText FormatSavedAtLocal(const FDateTime& savedAtUtc)
	{
		// An empty timezone asks ICU to use the platform's current local timezone and Culture.
		return FText::AsDateTime(savedAtUtc, EDateTimeStyle::Short, EDateTimeStyle::Short);
	}

	FText FormatSavedAtWithOffset(const FDateTime& savedAtUtc, const FTimespan localOffset)
	{
		return FText::AsDateTime(savedAtUtc + localOffset, EDateTimeStyle::Short, EDateTimeStyle::Short,
			FText::GetInvariantTimeZone());
	}

	FText FormatPlayTime(const double playTimeSeconds)
	{
		const int64 totalSeconds = FMath::Max<int64>(0, FMath::FloorToInt64(playTimeSeconds));
		const int64 hours = totalSeconds / 3600;
		const int64 minutes = (totalSeconds % 3600) / 60;
		const int64 seconds = totalSeconds % 60;
		return FText::FromString(FString::Printf(TEXT("%02lld:%02lld:%02lld"), hours, minutes, seconds));
	}

	FText FormatCollectedCount(const int32 collectedCount, const int32 totalCount)
	{
		return FText::Format(NSLOCTEXT("LRSaveUI", "CollectedCount", "{0}/{1}"),
			FText::AsNumber(FMath::Max(0, collectedCount)), FText::AsNumber(FMath::Max(0, totalCount)));
	}
}
