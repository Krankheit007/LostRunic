#include "Save/LRSaveSubsystem.h"

#include "Core/LRLog.h"
#include "Data/LRSaveTuning.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Save/LRSaveCatalog.h"
#include "Save/LRSaveCatalogStore.h"
#include "Save/LRSavePayload.h"
#include "Save/LRSaveProvider.h"
#include "TimerManager.h"

namespace
{
	constexpr int32 SaveUserIndex = 0;
}

FLRSaveSlotMetadata ULRSaveSubsystem::BuildMetadata(const FLRSaveSlotId& slotId, const int32 displayIndex,
	const int64 saveSequence, const FLRSaveDataV2& data, const FString& payloadKey) const
{
	FLRSaveSlotMetadata metadata;
	metadata.SlotId = slotId;
	metadata.DisplayIndex = displayIndex;
	metadata.PayloadKey = payloadKey;
	metadata.MapId = data.Player.CurrentMapId;
	metadata.SavedAtUtc = FDateTime::UtcNow();
	metadata.PlayTimeSeconds = data.Statistics.PlayTimeSeconds;
	metadata.SaveSequence = saveSequence;
	metadata.Health = ELRSaveSlotHealth::Healthy;
	metadata.CollectedCount = data.Collectible.CollectibleIds.Num();
	return metadata;
}

void ULRSaveSubsystem::StartWrite()
{
	if (ActivePayload && ActiveOperation.PayloadKey.Len() > 0
		&& OperationState == ELRSaveOperationState::WritingPayload)
	{
		FAsyncSaveGameToSlotDelegate saveDelegate = FAsyncSaveGameToSlotDelegate::CreateWeakLambda(this,
			[this, operationId = ActiveOperation.OperationId](const FString& slotName, const int32 userIndex,
				const bool bSuccess)
			{
				HandlePayloadWritten(operationId, slotName, userIndex, bSuccess);
			});
		UGameplayStatics::AsyncSaveGameToSlot(ActivePayload, ActiveOperation.PayloadKey, SaveUserIndex, saveDelegate);
		if (UWorld* world = GetCurrentWorld())
		{
			const FGuid operationId = ActiveOperation.OperationId;
			FTimerDelegate watchdog = FTimerDelegate::CreateWeakLambda(this,
				[this, operationId]() { HandleAsyncWatchdog(operationId); });
			world->GetTimerManager().SetTimer(AsyncWatchdogTimer, watchdog,
				GetEffectiveTuning().AsyncWatchdogSeconds, false);
		}
		return;
	}

	OperationState = ELRSaveOperationState::Capturing;
	if (!SaveCatalog || !ActiveOperation.bHasCapturedData)
	{
		CompleteOperation(ELRSaveResultCode::ProviderUnavailable,
			TEXT("Operation has no immutable V2 payload snapshot."));
		return;
	}
	const FLRSaveSlotMetadata* previous = SaveCatalog->FindSlot(ActiveOperation.SlotId);
	const bool bCanCreate = ActiveOperation.Type == ELRSaveOperationType::CreateManual;
	if (!bCanCreate && !previous)
	{
		CompleteOperation(ELRSaveResultCode::RejectedInvalidSlot, TEXT("Target slot does not exist."));
		return;
	}
	const int32 displayIndex = previous ? previous->DisplayIndex
		: (ActiveOperation.SlotId.Type == ELRSaveSlotType::Auto ? 0
			: SaveCatalog->FindLowestFreeDisplayIndex(GetManualSlotCount()));
	if (displayIndex == INDEX_NONE)
	{
		CompleteOperation(ELRSaveResultCode::RejectedAtCapacity, TEXT("Manual save capacity reached."));
		return;
	}

	int64 sequence = 1;
	for (const FLRSaveSlotMetadata& slot : SaveCatalog->Slots)
	{
		sequence = FMath::Max(sequence, slot.SaveSequence + 1);
	}
	ActiveOperation.CatalogSequence = sequence;
	ActiveOperation.PayloadKey = FLRSaveCatalogStore::MakePayloadKey(ActiveOperation.SlotId, sequence);
	ActivePayload = NewObject<ULRSavePayload>(this);
	ActivePayload->SlotId = ActiveOperation.SlotId;
	ActivePayload->PayloadKey = ActiveOperation.PayloadKey;
	ActivePayload->SaveSequence = sequence;
	ActivePayload->Data = ActiveOperation.CapturedData;
	ActivePayload->MetadataSnapshot = BuildMetadata(ActiveOperation.SlotId, displayIndex, sequence,
		ActivePayload->Data, ActiveOperation.PayloadKey);

	SaveCatalog->PendingOperation.Type = ELRCatalogPendingType::Write;
	SaveCatalog->PendingOperation.PreviousMetadata = previous ? *previous : FLRSaveSlotMetadata();
	SaveCatalog->PendingOperation.TargetMetadata = ActivePayload->MetadataSnapshot;
	OperationState = ELRSaveOperationState::CommittingCatalog;
	FString error;
	if (!FLRSaveCatalogStore::CommitCatalog(*SaveCatalog, error))
	{
		SaveCatalog->PendingOperation = FLRCatalogPendingOperation();
		CompleteOperation(ELRSaveResultCode::WriteFailed, error);
		return;
	}

	OperationState = ELRSaveOperationState::WritingPayload;
	FAsyncSaveGameToSlotDelegate saveDelegate = FAsyncSaveGameToSlotDelegate::CreateWeakLambda(this,
		[this, operationId = ActiveOperation.OperationId](const FString& slotName, const int32 userIndex,
			const bool bSuccess)
		{
			HandlePayloadWritten(operationId, slotName, userIndex, bSuccess);
		});
	UGameplayStatics::AsyncSaveGameToSlot(ActivePayload, ActiveOperation.PayloadKey, SaveUserIndex, saveDelegate);
	const FGuid operationId = ActiveOperation.OperationId;
	if (UWorld* world = GetCurrentWorld())
	{
		FTimerDelegate watchdog = FTimerDelegate::CreateWeakLambda(this,
			[this, operationId]() { HandleAsyncWatchdog(operationId); });
		world->GetTimerManager().SetTimer(AsyncWatchdogTimer, watchdog,
			GetEffectiveTuning().AsyncWatchdogSeconds, false);
	}
}

void ULRSaveSubsystem::HandlePayloadWritten(const FGuid operationId, const FString& slotName,
	const int32 userIndex, const bool bSuccess)
{
	if (ActiveOperation.OperationId != operationId || OperationState != ELRSaveOperationState::WritingPayload
		|| !ActivePayload || slotName != ActiveOperation.PayloadKey || userIndex != SaveUserIndex)
	{
		UE_LOG(LogLostRunicSave, VeryVerbose, TEXT("Ignored late save callback operation=%s slot=%s"),
			*operationId.ToString(), *slotName);
		return;
	}
	if (UWorld* world = GetCurrentWorld())
	{
		world->GetTimerManager().ClearTimer(AsyncWatchdogTimer);
	}
	if (!bSuccess)
	{
		if (ActiveOperation.RetryCount < GetEffectiveTuning().RetryCount)
		{
			++ActiveOperation.RetryCount;
			if (UWorld* world = GetCurrentWorld())
			{
				FTimerDelegate retry = FTimerDelegate::CreateWeakLambda(this,
					[this, operationId]() { RetryActiveOperation(operationId); });
				world->GetTimerManager().SetTimer(ExplicitRetryTimer, retry,
					GetEffectiveTuning().RetryDelaySeconds, false);
				return;
			}
		}
		const FLRCatalogPendingOperation pending = SaveCatalog->PendingOperation;
		SaveCatalog->PendingOperation = FLRCatalogPendingOperation();
		FString rollbackError;
		if (!FLRSaveCatalogStore::CommitCatalog(*SaveCatalog, rollbackError))
		{
			SaveCatalog->PendingOperation = pending;
			EnqueuePendingCatalogRepair();
		}
		CompleteOperation(ELRSaveResultCode::WriteFailed, TEXT("Payload write failed after retries."));
		return;
	}

	const FLRCatalogPendingOperation pending = SaveCatalog->PendingOperation;
	if (FLRSaveSlotMetadata* existing = SaveCatalog->FindSlot(pending.TargetMetadata.SlotId))
	{
		*existing = pending.TargetMetadata;
	}
	else
	{
		SaveCatalog->Slots.Add(pending.TargetMetadata);
	}
	SaveCatalog->PendingOperation = FLRCatalogPendingOperation();
	SaveCatalog->SortSlots();
	FString error;
	if (!FLRSaveCatalogStore::CommitCatalog(*SaveCatalog, error))
	{
		SaveCatalog->PendingOperation = pending;
		EnqueuePendingCatalogRepair();
		CompleteOperation(ELRSaveResultCode::WriteFailed, error);
		return;
	}
	CompleteOperation(ELRSaveResultCode::Succeeded);
}

void ULRSaveSubsystem::StartDelete()
{
	OperationState = ELRSaveOperationState::CommittingCatalog;
	const FLRSaveSlotMetadata* target = SaveCatalog ? SaveCatalog->FindSlot(ActiveOperation.SlotId) : nullptr;
	if (!target)
	{
		CompleteOperation(ELRSaveResultCode::RejectedInvalidSlot, TEXT("Target slot does not exist."));
		return;
	}
	const FLRSaveSlotMetadata targetMetadata = *target;
	SaveCatalog->PendingOperation.Type = ELRCatalogPendingType::Delete;
	SaveCatalog->PendingOperation.PreviousMetadata = targetMetadata;
	SaveCatalog->PendingOperation.TargetMetadata = targetMetadata;
	FString error;
	if (!FLRSaveCatalogStore::CommitCatalog(*SaveCatalog, error))
	{
		SaveCatalog->PendingOperation = FLRCatalogPendingOperation();
		CompleteOperation(ELRSaveResultCode::DeleteFailed, error);
		return;
	}
	OperationState = ELRSaveOperationState::DeletingPayload;
	if (!targetMetadata.PayloadKey.IsEmpty()
		&& UGameplayStatics::DoesSaveGameExist(targetMetadata.PayloadKey, SaveUserIndex)
		&& !UGameplayStatics::DeleteGameInSlot(targetMetadata.PayloadKey, SaveUserIndex))
	{
		EnqueuePendingCatalogRepair();
		CompleteOperation(ELRSaveResultCode::DeleteFailed,
			TEXT("Payload delete failed; pending delete retained for RepairHealth."));
		return;
	}
	SaveCatalog->Slots.RemoveAll([this](const FLRSaveSlotMetadata& slot)
	{
		return slot.SlotId == ActiveOperation.SlotId;
	});
	SaveCatalog->PendingOperation = FLRCatalogPendingOperation();
	if (!FLRSaveCatalogStore::CommitCatalog(*SaveCatalog, error))
	{
		SaveCatalog->PendingOperation.Type = ELRCatalogPendingType::Delete;
		SaveCatalog->PendingOperation.PreviousMetadata = targetMetadata;
		SaveCatalog->PendingOperation.TargetMetadata = targetMetadata;
		EnqueuePendingCatalogRepair();
		CompleteOperation(ELRSaveResultCode::DeleteFailed, error);
		return;
	}
	CompleteOperation(ELRSaveResultCode::Succeeded);
}

void ULRSaveSubsystem::StartRepairHealth()
{
	const bool bRecoveringCatalog = !ActiveOperation.SlotId.IsValid()
		&& ActiveOperation.RequestedHealth == ELRSaveSlotHealth::Healthy;
	OperationState = bRecoveringCatalog ? ELRSaveOperationState::RecoveringCatalog
		: ELRSaveOperationState::RepairingHealth;
	FString error;
	if (ActiveOperation.SlotId.IsValid())
	{
		FLRSaveSlotMetadata* slot = SaveCatalog ? SaveCatalog->FindSlot(ActiveOperation.SlotId) : nullptr;
		if (!slot)
		{
			CompleteOperation(ELRSaveResultCode::RejectedInvalidSlot, TEXT("Health repair target does not exist."));
			return;
		}
		slot->Health = ActiveOperation.RequestedHealth;
		if (!FLRSaveCatalogStore::CommitCatalog(*SaveCatalog, error))
		{
			CompleteOperation(ELRSaveResultCode::WriteFailed, error);
			return;
		}
		CompleteOperation(ELRSaveResultCode::Succeeded);
		return;
	}
	if (SaveCatalog && SaveCatalog->PendingOperation.IsSet())
	{
		if (!FLRSaveCatalogStore::RecoverPendingOperation(*SaveCatalog, error))
		{
			CompleteOperation(ELRSaveResultCode::WriteFailed, error);
			return;
		}
	}
	CompleteOperation(ELRSaveResultCode::Succeeded);
}
