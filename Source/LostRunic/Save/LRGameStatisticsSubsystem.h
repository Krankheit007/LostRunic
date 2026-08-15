/** @file LRGameStatisticsSubsystem.h @brief Event-driven persistent play-time and death statistics. */
#pragma once

#include "Subsystems/GameInstanceSubsystem.h"
#include "Save/LRSaveV2Types.h"

#include "LRGameStatisticsSubsystem.generated.h"

UCLASS()
class LOSTRUNIC_API ULRGameStatisticsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;
	void SetPlayTimeActive(bool bActive);
	void RecordDeath();
	double GetCurrentPlayTimeSeconds() const;
	void Capture(FLRSaveStatisticsChunk& outStatistics) const;
	void Restore(const FLRSaveStatisticsChunk& statistics);
	void ResetForNewGame();

private:
	void CommitActiveInterval();
	double AccumulatedPlayTimeSeconds = 0.0;
	double ActiveIntervalStartSeconds = 0.0;
	int32 DeathCount = 0;
	bool bPlayTimeActive = false;
};
