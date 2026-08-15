#include "Save/LRGameStatisticsSubsystem.h"

#include "HAL/PlatformTime.h"

void ULRGameStatisticsSubsystem::Deinitialize()
{
	CommitActiveInterval();
	Super::Deinitialize();
}

void ULRGameStatisticsSubsystem::SetPlayTimeActive(const bool bActive)
{
	if (bPlayTimeActive == bActive)
	{
		return;
	}
	if (bActive)
	{
		ActiveIntervalStartSeconds = FPlatformTime::Seconds();
		bPlayTimeActive = true;
	}
	else
	{
		CommitActiveInterval();
	}
}

void ULRGameStatisticsSubsystem::RecordDeath() { ++DeathCount; }

double ULRGameStatisticsSubsystem::GetCurrentPlayTimeSeconds() const
{
	return AccumulatedPlayTimeSeconds + (bPlayTimeActive ? FPlatformTime::Seconds() - ActiveIntervalStartSeconds : 0.0);
}

void ULRGameStatisticsSubsystem::Capture(FLRSaveStatisticsChunk& outStatistics) const
{
	outStatistics.DeathCount = DeathCount;
	outStatistics.PlayTimeSeconds = GetCurrentPlayTimeSeconds();
}

void ULRGameStatisticsSubsystem::Restore(const FLRSaveStatisticsChunk& statistics)
{
	AccumulatedPlayTimeSeconds = FMath::Max(0.0, statistics.PlayTimeSeconds);
	DeathCount = FMath::Max(0, statistics.DeathCount);
	ActiveIntervalStartSeconds = bPlayTimeActive ? FPlatformTime::Seconds() : 0.0;
}

void ULRGameStatisticsSubsystem::ResetForNewGame()
{
	AccumulatedPlayTimeSeconds = 0.0;
	DeathCount = 0;
	ActiveIntervalStartSeconds = bPlayTimeActive ? FPlatformTime::Seconds() : 0.0;
}

void ULRGameStatisticsSubsystem::CommitActiveInterval()
{
	if (bPlayTimeActive)
	{
		AccumulatedPlayTimeSeconds += FPlatformTime::Seconds() - ActiveIntervalStartSeconds;
		bPlayTimeActive = false;
		ActiveIntervalStartSeconds = 0.0;
	}
}
