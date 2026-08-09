#include "Framework/LRGameInstanceSubsystem.h"

#include "Core/LRLog.h"
#include "Data/LRGameContentSet.h"
#include "Data/LRGameTuningSet.h"
#include "Data/LRProjectSettings.h"

void ULRGameInstanceSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
	Super::Initialize(collection);

	const ULRProjectSettings* settings = GetDefault<ULRProjectSettings>();
	TuningSet = settings->TuningSet.LoadSynchronous();
	ContentSet = settings->ContentSet.LoadSynchronous();

	FString tuningError;
	FString contentError;
	bConfigurationValid = TuningSet && ContentSet
		&& TuningSet->Validate(tuningError)
		&& ContentSet->Validate(contentError);

	if (!bConfigurationValid)
	{
		UE_LOG(LogLostRunicTuning, Error, TEXT("GameInstance=%s invalid data roots. Tuning='%s' Content='%s'"),
			*GetNameSafe(GetGameInstance()), *tuningError, *contentError);
		return;
	}

	TuningSet->LogSources();
}

void ULRGameInstanceSubsystem::Deinitialize()
{
	bConfigurationValid = false;
	ContentSet = nullptr;
	TuningSet = nullptr;
	Super::Deinitialize();
}
