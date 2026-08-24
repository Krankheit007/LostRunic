#include "Framework/LRGameFlowSubsystem.h"

#include "Core/LRGameplayTags.h"
#include "Core/LRLog.h"
#include "Data/LRGameContentSet.h"
#include "Framework/LRCharacter.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Narrative/LRStoryStateSubsystem.h"
#include "Save/LRGameStatisticsSubsystem.h"
#include "Save/LRSaveSubsystem.h"
#include "Save/LRStorySaveAdapter.h"
#include "State/LRStateComponent.h"

void ULRGameFlowSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
	Super::Initialize(collection);
	collection.InitializeDependency<ULRGameInstanceSubsystem>();
	collection.InitializeDependency<ULRSaveSubsystem>();
	collection.InitializeDependency<ULRStoryStateSubsystem>();
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
	ClearStandardFlow();
	ClearMemoryTransaction();
	Super::Deinitialize();
}

FLRSaveOperationResult ULRGameFlowSubsystem::RequestLoadSave(const FLRSaveSlotId& slotId)
{
	ULRSaveSubsystem* save = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULRSaveSubsystem>() : nullptr;
	return save ? save->RequestLoadSaveForFlow(slotId, FGuid::NewGuid()) : FLRSaveOperationResult();
}

FLRSaveOperationResult ULRGameFlowSubsystem::RequestContinue()
{
	ULRSaveSubsystem* save = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULRSaveSubsystem>() : nullptr;
	return save ? save->RequestContinueForFlow(FGuid::NewGuid()) : FLRSaveOperationResult();
}

FLRSaveOperationResult ULRGameFlowSubsystem::RequestNewGame()
{
	ULRSaveSubsystem* save = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULRSaveSubsystem>() : nullptr;
	return save ? save->RequestNewGameForFlow(FGuid::NewGuid()) : FLRSaveOperationResult();
}

void ULRGameFlowSubsystem::HandleLoadRequested(const FLRSaveFlowRequest request)
{
	FString error;
	if (!ValidateFlowRequest(request, error))
	{
		if (ULRSaveSubsystem* save = GetGameInstance()->GetSubsystem<ULRSaveSubsystem>())
		{
			save->NotifyLoadPreparationFailed(request.GameFlowTransactionId, request.OperationId, error);
		}
		return;
	}
	ActiveFlowRequest = request;
	ActiveMapId = request.MapId;
	SetFlowPhase(request.GameFlowTransactionId, ELRGameFlowPhase::PreparingTravel, request.MapId);
	if (GetCurrentMapId() == request.MapId)
	{
		SetFlowPhase(request.GameFlowTransactionId, ELRGameFlowPhase::WaitingForWorld, request.MapId);
		return;
	}
	if (!TravelToMap(request.MapId))
	{
		if (ULRSaveSubsystem* save = GetGameInstance()->GetSubsystem<ULRSaveSubsystem>())
		{
			save->NotifyLoadPreparationFailed(request.GameFlowTransactionId, request.OperationId,
				TEXT("Load map travel could not be started."));
		}
	}
}

void ULRGameFlowSubsystem::HandleNewGameRequested(const FLRSaveFlowRequest request)
{
	FString error;
	if (!ValidateFlowRequest(request, error))
	{
		if (ULRSaveSubsystem* save = GetGameInstance()->GetSubsystem<ULRSaveSubsystem>())
		{
			save->NotifyLoadPreparationFailed(request.GameFlowTransactionId, request.OperationId, error);
		}
		return;
	}
	ActiveFlowRequest = request;
	ActiveMapId = request.MapId;
	SetFlowPhase(request.GameFlowTransactionId, ELRGameFlowPhase::PreparingTravel, request.MapId);
	if (GetCurrentMapId() == request.MapId)
	{
		SetFlowPhase(request.GameFlowTransactionId, ELRGameFlowPhase::WaitingForWorld, request.MapId);
		return;
	}
	if (!TravelToMap(request.MapId))
	{
		if (ULRSaveSubsystem* save = GetGameInstance()->GetSubsystem<ULRSaveSubsystem>())
		{
			save->NotifyLoadPreparationFailed(request.GameFlowTransactionId, request.OperationId,
				TEXT("New Game map travel could not be started."));
		}
	}
}

void ULRGameFlowSubsystem::NotifyWorldReady(ALRCharacter* character)
{
	const FName mapId = GetCurrentMapId();
	if (ActiveFlowRequest.OperationId.IsValid() && ActiveFlowRequest.MapId == mapId)
	{
		if (ULRSaveSubsystem* save = GetGameInstance()->GetSubsystem<ULRSaveSubsystem>())
		{
			SetFlowPhase(ActiveFlowRequest.GameFlowTransactionId, ELRGameFlowPhase::Restoring, mapId);
			if (ActiveFlowRequest.Operation == ELRSaveOperationType::NewGame)
			{
				save->NotifyNewGameWorldReady(ActiveFlowRequest.GameFlowTransactionId,
					ActiveFlowRequest.OperationId);
			}
			else
			{
				save->NotifyLoadWorldReady(ActiveFlowRequest.GameFlowTransactionId,
					ActiveFlowRequest.OperationId);
			}
		}
		return;
	}
	if (bHasHomeSnapshot && FlowPhase == ELRGameFlowPhase::WaitingForWorld
		&& mapId == LRSaveIds::MemoryMapId)
	{
		if (character)
		{
			if (ULRStateComponent* state = character->FindComponentByClass<ULRStateComponent>())
			{
				FLRStateChangeRequest request;
				request.TargetMode = ELRPerceptionMode::Memory;
				request.RequestType = ELRStateRequestType::Death;
				request.Source = LRGameplayTags::StateSourceDeath;
				state->RequestStateChange(request);
			}
		}
		SetFlowPhase(MemoryTransactionId, ELRGameFlowPhase::EnteringMemory, mapId);
		QueueMemoryCriticalSave(ELRSaveMemoryPurpose::Entry, LRSaveIds::MemoryEntryReason);
		return;
	}
	if (bHasHomeSnapshot && FlowPhase == ELRGameFlowPhase::ReturningFromMemory
		&& HomeSnapshot.Player.ResumeAnchor.MapId == mapId)
	{
		SetFlowPhase(MemoryTransactionId, ELRGameFlowPhase::Restoring, mapId);
		if (!RestoreHomeStateForReturn())
		{
			SetFlowPhase(MemoryTransactionId, ELRGameFlowPhase::ReturningFromMemory, mapId);
		}
	}
}

bool ULRGameFlowSubsystem::RequestEnterMemory(ALRCharacter* character)
{
	ULRSaveSubsystem* save = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULRSaveSubsystem>() : nullptr;
	if (!save || !character)
	{
		return false;
	}
	if (bHasHomeSnapshot)
	{
		if (GetCurrentMapId() == LRSaveIds::MemoryMapId && !bMemoryWorldActive)
		{
			return QueueMemoryCriticalSave(ELRSaveMemoryPurpose::Entry, LRSaveIds::MemoryEntryReason);
		}
		return false;
	}
	if (FlowPhase != ELRGameFlowPhase::Idle)
	{
		return false;
	}
	if (ULRGameStatisticsSubsystem* statistics = GetGameInstance()->GetSubsystem<ULRGameStatisticsSubsystem>())
	{
		statistics->RecordDeath();
	}
	FString error;
	FLRSaveDataV2 snapshot;
	if (!save->CaptureProviderState(snapshot, error) || !snapshot.Player.ResumeAnchor.IsValid())
	{
		UE_LOG(LogLostRunicSave, Warning, TEXT("Memory HomeSnapshot capture failed: %s"), *error);
		return false;
	}
	HomeSnapshot = snapshot;
	DurableNarrativeDelta = FLRNarrativePersistentDelta();
	bHasHomeSnapshot = true;
	bMemoryWorldActive = false;
	MemoryTransactionId = FGuid::NewGuid();
	SetFlowPhase(MemoryTransactionId, ELRGameFlowPhase::PreparingTravel, LRSaveIds::MemoryMapId);
	if (!TravelToMap(LRSaveIds::MemoryMapId))
	{
		SetFlowPhase(MemoryTransactionId, ELRGameFlowPhase::Idle, GetCurrentMapId());
		return false;
	}
	SetFlowPhase(MemoryTransactionId, ELRGameFlowPhase::Traveling, LRSaveIds::MemoryMapId);
	SetFlowPhase(MemoryTransactionId, ELRGameFlowPhase::WaitingForWorld, LRSaveIds::MemoryMapId);
	return true;
}

bool ULRGameFlowSubsystem::RequestCommitMemoryEvent(const FName eventId)
{
	if (!bHasHomeSnapshot || !bMemoryWorldActive || eventId.IsNone() || PendingMemorySaveOperationId.IsValid())
	{
		return false;
	}
	ULRStoryStateSubsystem* storyState = ULRStoryStateSubsystem::Resolve(GetGameInstance());
	FLRNarrativePersistentDelta delta;
	if (!storyState || !storyState->CommitMemoryEvent(eventId, &delta))
	{
		return false;
	}
	DurableNarrativeDelta.AddedStoryFlags.AppendTags(delta.AddedStoryFlags);
	DurableNarrativeDelta.AddedCompletedEventIds.Append(delta.AddedCompletedEventIds);
	DurableNarrativeDelta.AddedMemoryEventIds.Append(delta.AddedMemoryEventIds);
	return QueueMemoryCriticalSave(ELRSaveMemoryPurpose::Event, eventId);
}

bool ULRGameFlowSubsystem::RequestReturnFromMemory()
{
	if (!bHasHomeSnapshot || !bMemoryWorldActive || PendingMemorySaveOperationId.IsValid()
		|| !HomeSnapshot.Player.ResumeAnchor.IsValid())
	{
		return false;
	}
	const FName targetMapId = HomeSnapshot.Player.ResumeAnchor.MapId;
	SetFlowPhase(MemoryTransactionId, ELRGameFlowPhase::ReturningFromMemory, targetMapId);
	if (GetCurrentMapId() == targetMapId)
	{
		if (ALRCharacter* character = Cast<ALRCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
		{
			NotifyWorldReady(character);
			return true;
		}
	}
	if (!TravelToMap(targetMapId))
	{
		SetFlowPhase(MemoryTransactionId, ELRGameFlowPhase::Idle, GetCurrentMapId());
		return false;
	}
	return true;
}

bool ULRGameFlowSubsystem::QueueMemoryCriticalSave(const ELRSaveMemoryPurpose purpose, const FName reasonId)
{
	ULRSaveSubsystem* save = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULRSaveSubsystem>() : nullptr;
	if (!save || !bHasHomeSnapshot || !MemoryTransactionId.IsValid() || PendingMemorySaveOperationId.IsValid())
	{
		return false;
	}
	const FLRSaveOperationResult result = save->RequestCriticalSaveFromSnapshot(HomeSnapshot,
		DurableNarrativeDelta, reasonId, MemoryTransactionId, purpose);
	if (result.Code != ELRSaveResultCode::Queued)
	{
		return false;
	}
	PendingMemorySaveOperationId = result.OperationId;
	PendingMemoryPurpose = purpose;
	SetFlowPhase(MemoryTransactionId, PendingMemorySaveOperationId, FlowPhase, ActiveMapId);
	return true;
}

bool ULRGameFlowSubsystem::RestoreHomeStateForReturn()
{
	ULRSaveSubsystem* save = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULRSaveSubsystem>() : nullptr;
	ULRStoryStateSubsystem* storyState = ULRStoryStateSubsystem::Resolve(GetGameInstance());
	if (!save || !storyState)
	{
		return false;
	}
	FString error;
	if (!save->RestoreProviderState(HomeSnapshot, error))
	{
		UE_LOG(LogLostRunicSave, Warning, TEXT("Memory HomeSnapshot restore failed: %s"), *error);
		return false;
	}
	FLRNarrativePersistentState homeState;
	LRStorySaveAdapter::ToPersistentState(HomeSnapshot.Story, homeState);
	if (!storyState->ReplacePersistentState(homeState)
		|| !storyState->ApplyPersistentDelta(DurableNarrativeDelta))
	{
		UE_LOG(LogLostRunicNarrative, Warning, TEXT("Memory narrative restore rejected for transaction=%s"),
			*MemoryTransactionId.ToString());
		return false;
	}
	return QueueMemoryCriticalSave(ELRSaveMemoryPurpose::Return, LRSaveIds::MemoryReturnReason);
}

void ULRGameFlowSubsystem::HandleSaveOperationCompleted(const FLRSaveOperationResult result)
{
	if (ActiveFlowRequest.OperationId.IsValid()
		&& result.GameFlowTransactionId == ActiveFlowRequest.GameFlowTransactionId
		&& result.OperationId == ActiveFlowRequest.OperationId)
	{
		if (result.Code == ELRSaveResultCode::Succeeded)
		{
			SetFlowPhase(result.GameFlowTransactionId, result.OperationId, ELRGameFlowPhase::Idle, ActiveMapId);
			ClearStandardFlow();
		}
		else
		{
			SetFlowPhase(result.GameFlowTransactionId, result.OperationId, ELRGameFlowPhase::Idle, ActiveMapId);
			ClearStandardFlow();
		}
		return;
	}
	if (!MemoryTransactionId.IsValid()
		|| result.GameFlowTransactionId != MemoryTransactionId
		|| result.OperationId != PendingMemorySaveOperationId)
	{
		return;
	}
	const ELRSaveMemoryPurpose purpose = PendingMemoryPurpose;
	PendingMemorySaveOperationId.Invalidate();
	PendingMemoryPurpose = ELRSaveMemoryPurpose::None;
	if (result.Code != ELRSaveResultCode::Succeeded)
	{
		SetFlowPhase(MemoryTransactionId, ELRGameFlowPhase::Idle, GetCurrentMapId());
		return;
	}
	if (purpose == ELRSaveMemoryPurpose::Entry)
	{
		bMemoryWorldActive = true;
		SetFlowPhase(MemoryTransactionId, ELRGameFlowPhase::Idle, GetCurrentMapId());
	}
	else if (purpose == ELRSaveMemoryPurpose::Return)
	{
		const FGuid completedTransactionId = MemoryTransactionId;
		ClearMemoryTransaction();
		SetFlowPhase(completedTransactionId, result.OperationId, ELRGameFlowPhase::Idle, GetCurrentMapId());
	}
}

bool ULRGameFlowSubsystem::ValidateFlowRequest(const FLRSaveFlowRequest& request, FString& outError) const
{
	if (!request.GameFlowTransactionId.IsValid() || !request.OperationId.IsValid() || request.MapId.IsNone() || request.Operation == ELRSaveOperationType::None)
	{
		outError = TEXT("Save flow request is missing a transaction ID, operation ID, or map ID.");
		return false;
	}
	const ULRGameInstanceSubsystem* data = GetGameInstance()
		? GetGameInstance()->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	const ULRGameContentSet* content = data ? data->GetContentSet() : nullptr;
	if (!content || !content->FindMapRegistration(request.MapId))
	{
		outError = FString::Printf(TEXT("Map %s is not registered."), *request.MapId.ToString());
		return false;
	}
	return true;
}

void ULRGameFlowSubsystem::SetFlowPhase(const FGuid gameFlowTransactionId, const ELRGameFlowPhase newPhase,
	const FName mapId)
{
	FGuid saveOperationId;
	if (ActiveFlowRequest.GameFlowTransactionId == gameFlowTransactionId)
	{
		saveOperationId = ActiveFlowRequest.OperationId;
	}
	else if (MemoryTransactionId == gameFlowTransactionId)
	{
		saveOperationId = PendingMemorySaveOperationId;
	}
	SetFlowPhase(gameFlowTransactionId, saveOperationId, newPhase, mapId);
}

void ULRGameFlowSubsystem::SetFlowPhase(const FGuid gameFlowTransactionId, const FGuid saveOperationId,
	const ELRGameFlowPhase newPhase, const FName mapId)
{
	FlowPhase = newPhase;
	ActiveMapId = mapId;
	OnFlowPhaseChanged.Broadcast(gameFlowTransactionId, saveOperationId, newPhase, mapId);
}

void ULRGameFlowSubsystem::ClearStandardFlow()
{
	ActiveFlowRequest = FLRSaveFlowRequest();
	if (!bHasHomeSnapshot)
	{
		FlowPhase = ELRGameFlowPhase::Idle;
	}
}

void ULRGameFlowSubsystem::ClearMemoryTransaction()
{
	HomeSnapshot = FLRSaveDataV2();
	DurableNarrativeDelta = FLRNarrativePersistentDelta();
	bHasHomeSnapshot = false;
	bMemoryWorldActive = false;
	MemoryTransactionId.Invalidate();
	PendingMemorySaveOperationId.Invalidate();
	PendingMemoryPurpose = ELRSaveMemoryPurpose::None;
}

bool ULRGameFlowSubsystem::TravelToMainMenu()
{
	const ULRGameInstanceSubsystem* data = GetGameInstance()->GetSubsystem<ULRGameInstanceSubsystem>();
	const ULRGameContentSet* content = data ? data->GetContentSet() : nullptr;
	if (!content || content->MainMenuMapId.IsNone())
	{
		return false;
	}
	const FGuid transactionId = FGuid::NewGuid();
	SetFlowPhase(transactionId, ELRGameFlowPhase::PreparingTravel, content->MainMenuMapId);
	if (!TravelToMap(content->MainMenuMapId))
	{
		SetFlowPhase(transactionId, ELRGameFlowPhase::Idle, GetCurrentMapId());
		return false;
	}
	SetFlowPhase(transactionId, ELRGameFlowPhase::Traveling, content->MainMenuMapId);
	return true;
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

FName ULRGameFlowSubsystem::GetCurrentMapId() const
{
	const ULRGameInstanceSubsystem* data = GetGameInstance()
		? GetGameInstance()->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	const ULRGameContentSet* content = data ? data->GetContentSet() : nullptr;
	return content ? content->FindMapIdForWorld(GetWorld()) : NAME_None;
}
