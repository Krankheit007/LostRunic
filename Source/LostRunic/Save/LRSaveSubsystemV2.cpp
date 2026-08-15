#include "Save/LRSaveSubsystem.h"

#include "Core/LRLog.h"
#include "Data/LRGameContentSet.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Save/LRSaveCatalog.h"
#include "Save/LRSaveCatalogStore.h"
#include "Save/LRSaveOperationQueue.h"
#include "Save/LRSavePayload.h"
#include "Save/LRSaveProvider.h"

namespace
{
	constexpr int32 SaveUserIndex = 0;

	FLRSaveOperationResult MakeResult(const FLRQueuedSaveOperation& operation, const ELRSaveResultCode code,
		const FString& diagnostic = FString())
	{
		FLRSaveOperationResult result;
		result.OperationId = operation.OperationId;
		result.Operation = operation.Type;
		result.Code = code;
		result.SlotId = operation.SlotId;
		result.Diagnostic = diagnostic;
		return result;
	}
}

TArray<FLRSaveSlotMetadata> ULRSaveSubsystem::GetSaveSlots() const
{
	return SaveCatalog ? SaveCatalog->Slots : TArray<FLRSaveSlotMetadata>();
}

FLRSaveOperationResult ULRSaveSubsystem::RequestCreateManualSave(const FName reasonId)
{
	if (!IsManualSaveAllowed())
	{
		FLRQueuedSaveOperation rejected;
		rejected.Type = ELRSaveOperationType::CreateManual;
		return MakeResult(rejected, ELRSaveResultCode::RejectedNotEligible, TEXT("Manual save is not allowed."));
	}
	if (!SaveCatalog || SaveCatalog->FindLowestFreeDisplayIndex(GetManualSlotCount()) == INDEX_NONE)
	{
		FLRQueuedSaveOperation rejected;
		rejected.Type = ELRSaveOperationType::CreateManual;
		return MakeResult(rejected, ELRSaveResultCode::RejectedAtCapacity, TEXT("Manual save capacity reached."));
	}
	FLRSaveSlotId slotId;
	slotId.Type = ELRSaveSlotType::Manual;
	slotId.Guid = FGuid::NewGuid();
	return EnqueueV2Operation(ELRSaveOperationType::CreateManual, slotId, reasonId);
}

FLRSaveOperationResult ULRSaveSubsystem::RequestOverwriteSave(const FLRSaveSlotId slotId, const FName reasonId)
{
	if (!IsManualSaveAllowed())
	{
		FLRQueuedSaveOperation rejected;
		rejected.Type = ELRSaveOperationType::OverwriteManual;
		rejected.SlotId = slotId;
		return MakeResult(rejected, ELRSaveResultCode::RejectedNotEligible, TEXT("Manual save is not allowed."));
	}
	return EnqueueV2Operation(slotId.Type == ELRSaveSlotType::Auto
		? ELRSaveOperationType::AutoSave : ELRSaveOperationType::OverwriteManual, slotId, reasonId);
}

FLRSaveOperationResult ULRSaveSubsystem::RequestAutoSaveV2(const FName reasonId)
{
	FLRSaveSlotId slotId;
	slotId.Type = ELRSaveSlotType::Auto;
	slotId.Guid = LRSaveV2Ids::AutoSlotGuid;
	return EnqueueV2Operation(ELRSaveOperationType::AutoSave, slotId, reasonId);
}

FLRSaveOperationResult ULRSaveSubsystem::RequestLoadSave(const FLRSaveSlotId slotId)
{
	return EnqueueV2Operation(ELRSaveOperationType::Load, slotId);
}

FLRSaveOperationResult ULRSaveSubsystem::RequestDeleteSave(const FLRSaveSlotId slotId)
{
	if (slotId.Type == ELRSaveSlotType::Auto)
	{
		FLRQueuedSaveOperation rejected;
		rejected.Type = ELRSaveOperationType::Delete;
		rejected.SlotId = slotId;
		return MakeResult(rejected, ELRSaveResultCode::RejectedProtectedSlot, TEXT("Automatic slot cannot be deleted."));
	}
	return EnqueueV2Operation(ELRSaveOperationType::Delete, slotId);
}

FLRSaveOperationResult ULRSaveSubsystem::RequestContinue()
{
	FLRSaveSlotId noSlot;
	return EnqueueV2Operation(ELRSaveOperationType::Continue, noSlot);
}

FLRSaveOperationResult ULRSaveSubsystem::RequestNewGame()
{
	const ULRGameInstanceSubsystem* data = GetGameInstance()
		? GetGameInstance()->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	const ULRGameContentSet* content = data ? data->GetContentSet() : nullptr;
	FLRSaveOperationResult rejected;
	rejected.Operation = ELRSaveOperationType::NewGame;
	if (!content || content->NewGameMapId.IsNone())
	{
		rejected.Code = ELRSaveResultCode::RejectedNotEligible;
		rejected.Diagnostic = TEXT("New Game map is not configured.");
		return rejected;
	}
	FLRSaveSlotId slotId;
	slotId.Type = ELRSaveSlotType::Auto;
	slotId.Guid = LRSaveV2Ids::AutoSlotGuid;
	return EnqueueV2Operation(ELRSaveOperationType::NewGame, slotId, TEXT("NewGame"));
}

FLRSaveOperationResult ULRSaveSubsystem::EnqueueV2Operation(const ELRSaveOperationType type,
	const FLRSaveSlotId& slotId, const FName reasonId)
{
	FLRQueuedSaveOperation operation;
	operation.OperationId = FGuid::NewGuid();
	operation.Type = type;
	operation.SlotId = slotId;
	operation.ReasonId = reasonId;
	LRSaveOperationQueue::Enqueue(V2OperationQueue, MoveTemp(operation));
	const FLRSaveOperationResult result = MakeResult(V2OperationQueue.Last(), ELRSaveResultCode::Queued);
	StartNextV2Operation();
	return result;
}

void ULRSaveSubsystem::StartNextV2Operation()
{
	if (V2OperationState != ELRSaveOperationState::Idle || !SaveCatalog)
	{
		return;
	}
	if (SaveCatalog->PendingOperation.IsSet())
	{
		FString recoveryError;
		V2OperationState = ELRSaveOperationState::RecoveringCatalog;
		FLRSaveCatalogStore::RecoverPendingOperation(*SaveCatalog, recoveryError);
		V2OperationState = ELRSaveOperationState::Idle;
		if (SaveCatalog->PendingOperation.IsSet())
		{
			UE_LOG(LogLostRunicSave, Warning, TEXT("V2 queue blocked by pending catalog transaction: %s"),
				*recoveryError);
			return;
		}
	}
	if (!LRSaveOperationQueue::Dequeue(V2OperationQueue, ActiveV2Operation))
	{
		return;
	}
	switch (ActiveV2Operation.Type)
	{
	case ELRSaveOperationType::CreateManual:
	case ELRSaveOperationType::OverwriteManual:
	case ELRSaveOperationType::AutoSave:
	case ELRSaveOperationType::CriticalSave:
		StartV2Write();
		break;
	case ELRSaveOperationType::Load:
	case ELRSaveOperationType::Continue:
		StartV2Load();
		break;
	case ELRSaveOperationType::NewGame:
		StartV2NewGame();
		break;
	case ELRSaveOperationType::Delete:
		StartV2Delete();
		break;
	default:
		CompleteV2Operation(ELRSaveResultCode::InvalidData, TEXT("Unsupported V2 operation."));
		break;
	}
}

void ULRSaveSubsystem::CompleteV2Operation(const ELRSaveResultCode code, const FString& diagnostic)
{
	const FLRSaveOperationResult result = MakeResult(ActiveV2Operation, code, diagnostic);
	if (code == ELRSaveResultCode::Succeeded)
	{
		UE_LOG(LogLostRunicSave, Log, TEXT("Save V2 operation=%s type=%d completed."),
			*result.OperationId.ToString(), static_cast<int32>(result.Operation));
	}
	else
	{
		UE_LOG(LogLostRunicSave, Warning, TEXT("Save V2 operation=%s type=%d code=%d detail=%s"),
			*result.OperationId.ToString(), static_cast<int32>(result.Operation), static_cast<int32>(code), *diagnostic);
	}
	LoadedV2Payload = nullptr;
	ActiveV2Operation = FLRQueuedSaveOperation();
	V2OperationState = ELRSaveOperationState::Idle;
	OnSaveOperationCompleted.Broadcast(result);
	StartNextV2Operation();
}
