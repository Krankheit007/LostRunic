#include "Framework/LRGameFlowSubsystem.h"

#include "Core/LRLog.h"
#include "Data/LRGameContentSet.h"
#include "Data/LRGameTuningSet.h"
#include "Data/LRSaveTuning.h"
#include "Framework/LRCharacter.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Save/LRGameStatisticsSubsystem.h"
#include "Save/LRSaveSubsystem.h"

void ULRGameFlowSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
	Super::Initialize(collection);
	collection.InitializeDependency<ULRGameInstanceSubsystem>();
	collection.InitializeDependency<ULRSaveSubsystem>();
	collection.InitializeDependency<ULRGameStatisticsSubsystem>();
	if (ULRSaveSubsystem* save = GetGameInstance()->GetSubsystem<ULRSaveSubsystem>())
	{
		save->OnSaveLoadRequested.AddDynamic(this, &ULRGameFlowSubsystem::HandleLoadRequested);
		save->OnSaveNewGameRequested.AddDynamic(this, &ULRGameFlowSubsystem::HandleNewGameRequested);
		save->OnSaveOperationCompleted.AddDynamic(this, &ULRGameFlowSubsystem::HandleSaveOperationCompleted);
	}
}

void ULRGameFlowSubsystem::Deinitialize()
{
	if (ULRSaveSubsystem* save = GetGameInstance()->GetSubsystem<ULRSaveSubsystem>())
	{
		save->OnSaveLoadRequested.RemoveDynamic(this, &ULRGameFlowSubsystem::HandleLoadRequested);
		save->OnSaveNewGameRequested.RemoveDynamic(this, &ULRGameFlowSubsystem::HandleNewGameRequested);
		save->OnSaveOperationCompleted.RemoveDynamic(this, &ULRGameFlowSubsystem::HandleSaveOperationCompleted);
	}
	PendingLoadOperationId.Invalidate();
	PendingLoadMapId = NAME_None;
	PendingNewGameOperationId.Invalidate();
	PendingNewGameMapId = NAME_None;
	Super::Deinitialize();
}

void ULRGameFlowSubsystem::HandleNewGameRequested(const FGuid operationId, const FName mapId)
{
	ULRSaveSubsystem* save = GetGameInstance()->GetSubsystem<ULRSaveSubsystem>();
	const ULRGameInstanceSubsystem* data = GetGameInstance()->GetSubsystem<ULRGameInstanceSubsystem>();
	const ULRGameContentSet* content = data ? data->GetContentSet() : nullptr;
	if (!save || !content || !content->FindMapRegistration(mapId))
	{
		if (save)
		{
			save->NotifyLoadPreparationFailed(operationId, TEXT("New Game map is not registered."));
		}
		return;
	}
	PendingNewGameOperationId = operationId;
	PendingNewGameMapId = mapId;
	if (content->FindMapIdForWorld(GetWorld()) == mapId)
	{
		save->NotifyNewGameWorldReady(operationId);
		return;
	}
	if (!TravelToMap(mapId))
	{
		save->NotifyLoadPreparationFailed(operationId, TEXT("New Game map travel could not be started."));
	}
}

void ULRGameFlowSubsystem::HandleLoadRequested(const FGuid operationId, const FName mapId)
{
	ULRSaveSubsystem* save = GetGameInstance()->GetSubsystem<ULRSaveSubsystem>();
	const ULRGameInstanceSubsystem* data = GetGameInstance()->GetSubsystem<ULRGameInstanceSubsystem>();
	const ULRGameContentSet* content = data ? data->GetContentSet() : nullptr;
	if (!save || !content || !content->FindMapRegistration(mapId))
	{
		if (save)
		{
			save->NotifyLoadPreparationFailed(operationId, TEXT("Load map is not registered."));
		}
		return;
	}
	PendingLoadOperationId = operationId;
	PendingLoadMapId = mapId;
	if (content->FindMapIdForWorld(GetWorld()) == mapId)
	{
		save->NotifyLoadWorldReady(operationId);
		return;
	}
	if (!TravelToMap(mapId))
	{
		save->NotifyLoadPreparationFailed(operationId, TEXT("Map travel could not be started."));
	}
}

void ULRGameFlowSubsystem::NotifyWorldReady(ALRCharacter* character)
{
	ULRSaveSubsystem* save = GetGameInstance()->GetSubsystem<ULRSaveSubsystem>();
	const ULRGameInstanceSubsystem* data = GetGameInstance()->GetSubsystem<ULRGameInstanceSubsystem>();
	const ULRGameContentSet* content = data ? data->GetContentSet() : nullptr;
	const FLRMapRegistration* map = content ? content->FindMapRegistration(
		content->FindMapIdForWorld(GetWorld())) : nullptr;
	if (save && !PendingNewGameOperationId.IsValid())
	{
		save->HandleWorldReady(character);
	}
	const bool bRestoring = save && PendingLoadOperationId.IsValid()
		&& map && map->MapId == PendingLoadMapId;
	if (bRestoring)
	{
		const FGuid operationId = PendingLoadOperationId;
		save->NotifyLoadWorldReady(operationId);
	}
	const bool bStartingNewGame = save && PendingNewGameOperationId.IsValid()
		&& map && map->MapId == PendingNewGameMapId;
	if (bStartingNewGame)
	{
		const FGuid operationId = PendingNewGameOperationId;
		save->NotifyNewGameWorldReady(operationId);
		PendingNewGameOperationId.Invalidate();
		PendingNewGameMapId = NAME_None;
	}
	if (ULRGameStatisticsSubsystem* statistics = GetGameInstance()->GetSubsystem<ULRGameStatisticsSubsystem>())
	{
		statistics->SetPlayTimeActive(map && map->bPlayableMap);
	}
	const ULRGameTuningSet* tuningSet = data ? data->GetTuningSet() : nullptr;
	const ULRSaveTuning* tuning = tuningSet ? tuningSet->Save : nullptr;
	if (save && map && map->bPlayableMap && tuning && tuning->bAutoSaveAfterMapReady
		&& !PendingLoadOperationId.IsValid() && !PendingNewGameOperationId.IsValid())
	{
		save->RequestAutoSaveV2(TEXT("MapReady"));
	}
}

void ULRGameFlowSubsystem::HandleSaveOperationCompleted(const FLRSaveOperationResult result)
{
	if (result.OperationId == PendingLoadOperationId)
	{
		PendingLoadOperationId.Invalidate();
		PendingLoadMapId = NAME_None;
	}
	if (result.OperationId == PendingNewGameOperationId)
	{
		PendingNewGameOperationId.Invalidate();
		PendingNewGameMapId = NAME_None;
	}
}

bool ULRGameFlowSubsystem::TravelToMap(const FName mapId)
{
	const ULRGameInstanceSubsystem* data = GetGameInstance()->GetSubsystem<ULRGameInstanceSubsystem>();
	const ULRGameContentSet* content = data ? data->GetContentSet() : nullptr;
	const TSoftObjectPtr<UWorld> map = content ? content->FindMap(mapId) : TSoftObjectPtr<UWorld>();
	if (map.IsNull())
	{
		UE_LOG(LogLostRunicSave, Warning, TEXT("GameFlow rejected unregistered map=%s."), *mapId.ToString());
		return false;
	}
	if (ULRGameStatisticsSubsystem* statistics = GetGameInstance()->GetSubsystem<ULRGameStatisticsSubsystem>())
	{
		statistics->SetPlayTimeActive(false);
	}
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, map);
	return true;
}
