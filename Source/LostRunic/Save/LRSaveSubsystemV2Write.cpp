#include "Save/LRSaveSubsystem.h"

#include "Data/LRSaveTuning.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Save/LRSaveCatalog.h"
#include "Save/LRSaveCatalogStore.h"
#include "Save/LRSavePayload.h"
#include "Save/LRSaveProvider.h"

namespace
{
	constexpr int32 SaveUserIndex = 0;
}

FLRSaveSlotMetadata ULRSaveSubsystem::BuildV2Metadata(const FLRSaveSlotId& slotId, const int32 displayIndex,
	const int64 saveSequence, const ULRSavePayload& payload) const
{
	FLRSaveSlotMetadata metadata;
	metadata.SlotId = slotId;
	metadata.DisplayIndex = displayIndex;
	metadata.PayloadKey = payload.PayloadKey;
	metadata.MapId = payload.Data.Player.CurrentMapId;
	metadata.SavedAtUtc = FDateTime::UtcNow();
	metadata.PlayTimeSeconds = payload.Data.Statistics.PlayTimeSeconds;
	metadata.SaveSequence = saveSequence;
	metadata.Health = ELRSaveSlotHealth::Healthy;
	return metadata;
}

bool ULRSaveSubsystem::PrepareV2Payload(FLRQueuedSaveOperation& operation, const int32 displayIndex,
	FString& outError)
{
	UGameInstance* gameInstance = GetGameInstance();
	if (!gameInstance)
	{
		outError = TEXT("GameInstance is unavailable.");
		return false;
	}
	int64 sequence = 1;
	for (const FLRSaveSlotMetadata& slot : SaveCatalog->Slots)
	{
		sequence = FMath::Max(sequence, slot.SaveSequence + 1);
	}
	ULRSavePayload* payload = NewObject<ULRSavePayload>(this);
	payload->SlotId = operation.SlotId;
	payload->SaveSequence = sequence;
	payload->PayloadKey = FLRSaveCatalogStore::MakePayloadKey(operation.SlotId, sequence);
	if (!LRSaveProviders::CaptureAll(SaveProviders, *gameInstance, payload->Data, outError))
	{
		return false;
	}
	payload->MetadataSnapshot = BuildV2Metadata(operation.SlotId, displayIndex, sequence, *payload);
	operation.Payload = payload;
	LoadedV2Payload = payload;
	return true;
}

void ULRSaveSubsystem::StartV2Write()
{
	V2OperationState = ELRSaveOperationState::Capturing;
	const FLRSaveSlotMetadata* previous = SaveCatalog->FindSlot(ActiveV2Operation.SlotId);
	if (ActiveV2Operation.Type != ELRSaveOperationType::CreateManual
		&& ActiveV2Operation.Type != ELRSaveOperationType::AutoSave
		&& ActiveV2Operation.Type != ELRSaveOperationType::NewGame && !previous)
	{
		CompleteV2Operation(ELRSaveResultCode::RejectedInvalidSlot, TEXT("Target slot does not exist."));
		return;
	}
	const int32 displayIndex = previous ? previous->DisplayIndex
		: (ActiveV2Operation.SlotId.Type == ELRSaveSlotType::Auto ? 0
			: SaveCatalog->FindLowestFreeDisplayIndex(GetEffectiveTuning().MaxManualSaveSlots));
	if (displayIndex == INDEX_NONE)
	{
		CompleteV2Operation(ELRSaveResultCode::RejectedAtCapacity, TEXT("Manual save capacity reached."));
		return;
	}
	FString error;
	if (!PrepareV2Payload(ActiveV2Operation, displayIndex, error))
	{
		CompleteV2Operation(ELRSaveResultCode::ProviderUnavailable, error);
		return;
	}
	SaveCatalog->PendingOperation.Type = ELRCatalogPendingType::Write;
	SaveCatalog->PendingOperation.PreviousMetadata = previous ? *previous : FLRSaveSlotMetadata();
	SaveCatalog->PendingOperation.TargetMetadata = LoadedV2Payload->MetadataSnapshot;
	V2OperationState = ELRSaveOperationState::CommittingCatalog;
	if (!FLRSaveCatalogStore::CommitCatalog(*SaveCatalog, error))
	{
		SaveCatalog->PendingOperation = FLRCatalogPendingOperation();
		CompleteV2Operation(ELRSaveResultCode::WriteFailed, error);
		return;
	}
	V2OperationState = ELRSaveOperationState::WritingPayload;
	FAsyncSaveGameToSlotDelegate saveDelegate;
	saveDelegate.BindUObject(this, &ULRSaveSubsystem::HandleV2PayloadWritten);
	UGameplayStatics::AsyncSaveGameToSlot(LoadedV2Payload, LoadedV2Payload->PayloadKey,
		SaveUserIndex, saveDelegate);
}

void ULRSaveSubsystem::HandleV2PayloadWritten(const FString& slotName, const int32 userIndex,
	const bool bSuccess)
{
	if (V2OperationState != ELRSaveOperationState::WritingPayload || !LoadedV2Payload)
	{
		return;
	}
	if (!bSuccess)
	{
		FString rollbackError;
		SaveCatalog->PendingOperation = FLRCatalogPendingOperation();
		FLRSaveCatalogStore::CommitCatalog(*SaveCatalog, rollbackError);
		CompleteV2Operation(ELRSaveResultCode::WriteFailed, TEXT("Payload write failed."));
		return;
	}
	const FLRSaveSlotMetadata previous = SaveCatalog->PendingOperation.PreviousMetadata;
	const FLRSaveSlotMetadata target = SaveCatalog->PendingOperation.TargetMetadata;
	if (FLRSaveSlotMetadata* existing = SaveCatalog->FindSlot(target.SlotId))
	{
		*existing = target;
	}
	else
	{
		SaveCatalog->Slots.Add(target);
	}
	SaveCatalog->PendingOperation = FLRCatalogPendingOperation();
	SaveCatalog->SortSlots();
	V2OperationState = ELRSaveOperationState::CommittingCatalog;
	FString error;
	if (!FLRSaveCatalogStore::CommitCatalog(*SaveCatalog, error))
	{
		CompleteV2Operation(ELRSaveResultCode::WriteFailed, error);
		return;
	}
	if (!previous.PayloadKey.IsEmpty() && previous.PayloadKey != target.PayloadKey)
	{
		UGameplayStatics::DeleteGameInSlot(previous.PayloadKey, SaveUserIndex);
	}
	CompleteV2Operation(ELRSaveResultCode::Succeeded);
}

void ULRSaveSubsystem::StartV2Delete()
{
	const FLRSaveSlotMetadata* target = SaveCatalog->FindSlot(ActiveV2Operation.SlotId);
	if (!target)
	{
		CompleteV2Operation(ELRSaveResultCode::RejectedInvalidSlot, TEXT("Target slot does not exist."));
		return;
	}
	SaveCatalog->PendingOperation.Type = ELRCatalogPendingType::Delete;
	SaveCatalog->PendingOperation.PreviousMetadata = *target;
	SaveCatalog->PendingOperation.TargetMetadata = *target;
	V2OperationState = ELRSaveOperationState::CommittingCatalog;
	FString error;
	if (!FLRSaveCatalogStore::CommitCatalog(*SaveCatalog, error))
	{
		SaveCatalog->PendingOperation = FLRCatalogPendingOperation();
		CompleteV2Operation(ELRSaveResultCode::DeleteFailed, error);
		return;
	}
	V2OperationState = ELRSaveOperationState::DeletingPayload;
	if (UGameplayStatics::DoesSaveGameExist(target->PayloadKey, SaveUserIndex)
		&& !UGameplayStatics::DeleteGameInSlot(target->PayloadKey, SaveUserIndex))
	{
		CompleteV2Operation(ELRSaveResultCode::DeleteFailed, TEXT("Payload delete failed; pending delete retained."));
		return;
	}
	SaveCatalog->Slots.RemoveAll([this](const FLRSaveSlotMetadata& slot)
	{
		return slot.SlotId == ActiveV2Operation.SlotId;
	});
	SaveCatalog->PendingOperation = FLRCatalogPendingOperation();
	if (!FLRSaveCatalogStore::CommitCatalog(*SaveCatalog, error))
	{
		CompleteV2Operation(ELRSaveResultCode::DeleteFailed, error);
		return;
	}
	CompleteV2Operation(ELRSaveResultCode::Succeeded);
}
