#include "Save/LRSaveSubsystem.h"

#include "Core/LRLog.h"
#include "Data/LRGameContentSet.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "Save/LRSaveCatalog.h"
#include "Save/LRSaveCatalogStore.h"
#include "Save/LRSavePayload.h"
#include "Save/LRSaveProvider.h"
#include "Save/LRSaveRules.h"

void ULRSaveSubsystem::StartLoad()
{
	SetOperationState(ELRSaveOperationState::ReadingPayload);
	TArray<FLRSaveSlotMetadata> candidates;
	if (ActiveOperation.Type == ELRSaveOperationType::Continue)
	{
		FLRSaveSlotId candidateId;
		if (LRSaveRules::ResolveContinueCandidate(SaveCatalog->Slots, candidateId))
		{
			if (const FLRSaveSlotMetadata* candidate = SaveCatalog->FindSlot(candidateId))
			{
				candidates.Add(*candidate);
			}
		}
	}
	else if (const FLRSaveSlotMetadata* target = SaveCatalog->FindSlot(ActiveOperation.SlotId))
	{
		candidates.Add(*target);
	}
	if (candidates.IsEmpty())
	{
		CompleteOperation(ELRSaveResultCode::RejectedInvalidSlot, TEXT("No matching V2 save slot."));
		return;
	}

	ELRSaveResultCode lastCode = ELRSaveResultCode::ReadFailed;
	FString lastError;
	for (const FLRSaveSlotMetadata& metadata : candidates)
	{
		ELRSaveSlotHealth health = ELRSaveSlotHealth::Healthy;
		ActivePayload = FLRSaveCatalogStore::LoadAndValidatePayload(this, metadata, health, lastCode, lastError);
		if (ActivePayload)
		{
			ActiveOperation.SlotId = metadata.SlotId;
			SetOperationState(ELRSaveOperationState::AwaitingWorld);
			FLRSaveFlowRequest request;
			request.GameFlowTransactionId = ActiveOperation.GameFlowTransactionId;
			request.OperationId = ActiveOperation.OperationId;
			request.MapId = ActivePayload->Data.Player.CurrentMapId;
			request.Operation = ActiveOperation.Type;
			OnSaveLoadRequested.Broadcast(request);
			return;
		}
		if (FLRSaveCatalogStore::IsDeterministicHealth(health))
		{
			EnqueueHealthRepair(metadata.SlotId, health);
		}
	}
	CompleteOperation(lastCode, lastError);
}

void ULRSaveSubsystem::NotifyLoadWorldReady(const FGuid gameFlowTransactionId, const FGuid operationId)
{
	if (OperationState != ELRSaveOperationState::AwaitingWorld
		|| ActiveOperation.GameFlowTransactionId != gameFlowTransactionId
		|| ActiveOperation.OperationId != operationId || !ActivePayload || !GetGameInstance())
	{
		return;
	}
	SetOperationState(ELRSaveOperationState::Restoring);
	FString error;
	if (!LRSaveProviders::RestoreNonPlayer(SaveProviders, *GetGameInstance(), ActivePayload->Data, error)
		|| !LRSaveProviders::RestorePlayer(SaveProviders, *GetGameInstance(), ActivePayload->Data, error))
	{
		CompleteOperation(ELRSaveResultCode::ProviderUnavailable, error);
		return;
	}
	CurrentData = ActivePayload->Data;
	CompleteOperation(ELRSaveResultCode::Succeeded);
}

void ULRSaveSubsystem::NotifyLoadPreparationFailed(const FGuid gameFlowTransactionId, const FGuid operationId,
	const FString& diagnostic)
{
	if (OperationState == ELRSaveOperationState::AwaitingWorld
		&& ActiveOperation.GameFlowTransactionId == gameFlowTransactionId
		&& ActiveOperation.OperationId == operationId)
	{
		CompleteOperation(ELRSaveResultCode::RejectedNotEligible, diagnostic);
	}
}
void ULRSaveSubsystem::StartNewGame()
{
	const ULRGameInstanceSubsystem* dataSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	const ULRGameContentSet* content = dataSubsystem ? dataSubsystem->GetContentSet() : nullptr;
	if (!content || content->NewGameMapId.IsNone())
	{
		CompleteOperation(ELRSaveResultCode::RejectedNotEligible, TEXT("New Game map is not configured."));
		return;
	}
	SetOperationState(ELRSaveOperationState::AwaitingWorld);
	FLRSaveFlowRequest request;
	request.GameFlowTransactionId = ActiveOperation.GameFlowTransactionId;
	request.OperationId = ActiveOperation.OperationId;
	request.MapId = content->NewGameMapId;
	request.Operation = ActiveOperation.Type;
	OnSaveNewGameRequested.Broadcast(request);
}

void ULRSaveSubsystem::NotifyNewGameWorldReady(const FGuid gameFlowTransactionId, const FGuid operationId)
{
	if (OperationState != ELRSaveOperationState::AwaitingWorld
		|| ActiveOperation.GameFlowTransactionId != gameFlowTransactionId
		|| ActiveOperation.OperationId != operationId || !GetGameInstance())
	{
		return;
	}
	FString error;
	if (!ResetProvidersForNewGame(error) || !CaptureProviderState(ActiveOperation.CapturedData, error))
	{
		CompleteOperation(ELRSaveResultCode::ProviderUnavailable, error);
		return;
	}
	ActiveOperation.bHasCapturedData = true;
	CurrentData = ActiveOperation.CapturedData;
	StartWrite();
}
