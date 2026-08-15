#include "Save/LRSaveSubsystem.h"

#include "Core/LRLog.h"
#include "Data/LRGameContentSet.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "Save/LRSaveCatalog.h"
#include "Save/LRSaveCatalogStore.h"
#include "Save/LRSaveOperationQueue.h"
#include "Save/LRSaveRules.h"
#include "Data/LRSaveTuning.h"
#include "TimerManager.h"

namespace
{
	FLRSaveOperationResult MakeOperationResult(const FGuid operationId, const ELRSaveOperationType type,
		const FLRSaveSlotId& slotId, const ELRSaveResultCode code, const FString& diagnostic)
	{
		FLRSaveOperationResult result;
		result.OperationId = operationId;
		result.Operation = type;
		result.SlotId = slotId;
		result.Code = code;
		result.Diagnostic = diagnostic;
		return result;
	}

	FLRSaveSlotId MakeAutoSlotId()
	{
		FLRSaveSlotId slotId;
		slotId.Type = ELRSaveSlotType::Auto;
		slotId.Guid = LRSaveV2Ids::AutoSlotGuid;
		return slotId;
	}
}

FLRSaveOperationResult ULRSaveSubsystem::EnqueueOperation(const ELRSaveOperationType type,
	const FLRSaveSlotId& slotId, const FName reasonId, const FLRSaveDataV2* capturedData,
	const ELRSaveMemoryPurpose memoryPurpose, const ELRSaveSlotHealth requestedHealth,
	const bool bFront, const FGuid requestedOperationId)
{
	FLRQueuedSaveOperation operation;
	operation.OperationId = requestedOperationId.IsValid() ? requestedOperationId : FGuid::NewGuid();
	operation.Type = type;
	operation.SlotId = slotId;
	operation.ReasonId = reasonId;
	operation.MemoryPurpose = memoryPurpose;
	operation.RequestedHealth = requestedHealth;
	if (capturedData)
	{
		operation.CapturedData = *capturedData;
		operation.bHasCapturedData = true;
	}
	const FLRSaveOperationResult result = MakeOperationResult(operation.OperationId, operation.Type,
		operation.SlotId, ELRSaveResultCode::Queued, FString());
	if (bFront)
	{
		LRSaveOperationQueue::EnqueueFront(OperationQueue, MoveTemp(operation));
	}
	else
	{
		LRSaveOperationQueue::Enqueue(OperationQueue, MoveTemp(operation));
	}
	StartNextOperation();
	return result;
}
void ULRSaveSubsystem::EnqueuePendingCatalogRepair()
{
	if (!SaveCatalog || !SaveCatalog->PendingOperation.IsSet())
	{
		return;
	}
	EnqueueOperation(ELRSaveOperationType::RepairHealth, FLRSaveSlotId(), TEXT("CatalogRecovery"),
		nullptr, ELRSaveMemoryPurpose::None, ELRSaveSlotHealth::Healthy, true);
}

void ULRSaveSubsystem::EnqueueHealthRepair(const FLRSaveSlotId& slotId, const ELRSaveSlotHealth health)
{
	if (health == ELRSaveSlotHealth::Healthy)
	{
		return;
	}
	EnqueueOperation(ELRSaveOperationType::RepairHealth, slotId, TEXT("HealthRepair"), nullptr,
		ELRSaveMemoryPurpose::None, health, true);
}

void ULRSaveSubsystem::StartNextOperation()
{
	if (OperationState != ELRSaveOperationState::Idle || !SaveCatalog || OperationQueue.IsEmpty())
	{
		return;
	}
	if (!LRSaveOperationQueue::Dequeue(OperationQueue, ActiveOperation))
	{
		return;
	}
	OperationState = ELRSaveOperationState::Capturing;
	const FGuid operationId = ActiveOperation.OperationId;
	if (UWorld* world = GetCurrentWorld())
	{
		FTimerDelegate timeout = FTimerDelegate::CreateWeakLambda(this,
			[this, operationId]() { HandleOperationTimeout(operationId); });
		world->GetTimerManager().SetTimer(OperationTimeoutTimer, timeout,
			GetEffectiveTuning().OperationTimeoutSeconds, false);
	}
	DispatchActiveOperation();
}

void ULRSaveSubsystem::DispatchActiveOperation()
{
	switch (ActiveOperation.Type)
	{
	case ELRSaveOperationType::CreateManual:
	case ELRSaveOperationType::OverwriteManual:
	case ELRSaveOperationType::AutoSave:
	case ELRSaveOperationType::CriticalSave:
		StartWrite();
		break;
	case ELRSaveOperationType::Load:
	case ELRSaveOperationType::Continue:
		StartLoad();
		break;
	case ELRSaveOperationType::NewGame:
		StartNewGame();
		break;
	case ELRSaveOperationType::Delete:
		StartDelete();
		break;
	case ELRSaveOperationType::RepairHealth:
		StartRepairHealth();
		break;
	default:
		CompleteOperation(ELRSaveResultCode::InvalidData, TEXT("Unsupported save operation."));
		break;
	}
}

void ULRSaveSubsystem::HandleOperationTimeout(const FGuid operationId)
{
	if (ActiveOperation.OperationId != operationId || OperationState == ELRSaveOperationState::Idle)
	{
		return;
	}
	UE_LOG(LogLostRunicSave, Warning, TEXT("Save operation timed out operation=%s type=%d state=%d"),
		*operationId.ToString(), static_cast<int32>(ActiveOperation.Type), static_cast<int32>(OperationState));
	CompleteOperation(ELRSaveResultCode::TimedOut, TEXT("Save operation exceeded OperationTimeoutSeconds."));
}

void ULRSaveSubsystem::HandleAsyncWatchdog(const FGuid operationId)
{
	if (ActiveOperation.OperationId != operationId || OperationState != ELRSaveOperationState::WritingPayload)
	{
		return;
	}
	UE_LOG(LogLostRunicSave, Warning, TEXT("Save async watchdog timed out operation=%s"), *operationId.ToString());
	CompleteOperation(ELRSaveResultCode::TimedOut, TEXT("Async save callback exceeded AsyncWatchdogSeconds."));
}

void ULRSaveSubsystem::RetryActiveOperation(const FGuid operationId)
{
	if (ActiveOperation.OperationId != operationId || OperationState != ELRSaveOperationState::WritingPayload)
	{
		return;
	}
	StartWrite();
}

void ULRSaveSubsystem::CancelQueuedOperations(const FString& diagnostic)
{
	FLRQueuedSaveOperation operation;
	while (LRSaveOperationQueue::Dequeue(OperationQueue, operation))
	{
		OnSaveOperationCompleted.Broadcast(MakeOperationResult(operation.OperationId, operation.Type,
			operation.SlotId, ELRSaveResultCode::Cancelled, diagnostic));
	}
}

void ULRSaveSubsystem::CompleteOperation(const ELRSaveResultCode code, const FString& diagnostic)
{
	if (!ActiveOperation.OperationId.IsValid())
	{
		return;
	}
	if (UWorld* world = GetCurrentWorld())
	{
		FTimerManager& timers = world->GetTimerManager();
		timers.ClearTimer(OperationTimeoutTimer);
		timers.ClearTimer(AsyncWatchdogTimer);
		timers.ClearTimer(ExplicitRetryTimer);
	}
	const FLRQueuedSaveOperation completedOperation = ActiveOperation;
	const bool bSucceeded = code == ELRSaveResultCode::Succeeded;
	UpdateMemoryPhaseAfterOperation(completedOperation, bSucceeded);
	const bool bRecoveryOperation = completedOperation.Type == ELRSaveOperationType::RepairHealth
		&& completedOperation.SlotId.IsValid() == false
		&& completedOperation.RequestedHealth == ELRSaveSlotHealth::Healthy;
	if (bRecoveryOperation && !bSucceeded)
	{
		bPersistenceBlocked = true;
	}
	const FLRSaveOperationResult result = MakeOperationResult(completedOperation.OperationId,
		completedOperation.Type, completedOperation.SlotId, code, diagnostic);
	if (code == ELRSaveResultCode::Succeeded)
	{
		UE_LOG(LogLostRunicSave, Log, TEXT("Save operation=%s type=%d code=%d reason=%s detail=%s"),
			*result.OperationId.ToString(), static_cast<int32>(result.Operation), static_cast<int32>(result.Code),
			*completedOperation.ReasonId.ToString(), *diagnostic);
	}
	else
	{
		UE_LOG(LogLostRunicSave, Warning, TEXT("Save operation=%s type=%d code=%d reason=%s detail=%s"),
			*result.OperationId.ToString(), static_cast<int32>(result.Operation), static_cast<int32>(result.Code),
			*completedOperation.ReasonId.ToString(), *diagnostic);
	}
	ActiveOperation = FLRQueuedSaveOperation();
	ActivePayload = nullptr;
	OperationState = ELRSaveOperationState::Idle;
	OnSaveOperationCompleted.Broadcast(result);

	if (bRecoveryOperation && !bSucceeded)
	{
		CancelQueuedOperations(TEXT("Catalog recovery failed; queued persistence operations were cancelled."));
		return;
	}
	if (code == ELRSaveResultCode::TimedOut && SaveCatalog && SaveCatalog->PendingOperation.IsSet())
	{
		EnqueuePendingCatalogRepair();
	}
	StartNextOperation();
}

FLRSaveOperationResult ULRSaveSubsystem::RequestCreateManualSave(const FName reasonId)
{
	if (bPersistenceBlocked)
	{
		return MakeRejected(ELRSaveOperationType::CreateManual, FLRSaveSlotId(), ELRSaveResultCode::RejectedBusy,
			TEXT("Persistence is blocked until catalog recovery succeeds."));
	}
	if (!IsManualSaveAllowed())
	{
		return MakeRejected(ELRSaveOperationType::CreateManual, FLRSaveSlotId(), ELRSaveResultCode::RejectedNotEligible,
			TEXT("Manual save requires paused gameplay outside Memory."));
	}
	if (!SaveCatalog || SaveCatalog->FindLowestFreeDisplayIndex(GetManualSlotCount()) == INDEX_NONE)
	{
		return MakeRejected(ELRSaveOperationType::CreateManual, FLRSaveSlotId(), ELRSaveResultCode::RejectedAtCapacity,
			TEXT("Manual save capacity reached."));
	}
	FLRSaveDataV2 captured;
	FString error;
	if (!CaptureCurrentData(captured, error))
	{
		return MakeRejected(ELRSaveOperationType::CreateManual, FLRSaveSlotId(),
			ELRSaveResultCode::ProviderUnavailable, error);
	}
	FLRSaveSlotId slotId;
	slotId.Type = ELRSaveSlotType::Manual;
	slotId.Guid = FGuid::NewGuid();
	return EnqueueOperation(ELRSaveOperationType::CreateManual, slotId, reasonId, &captured);
}

FLRSaveOperationResult ULRSaveSubsystem::RequestOverwriteSave(const FLRSaveSlotId slotId, const FName reasonId)
{
	if (LRSaveRules::IsProtectedOverwrite(slotId))
	{
		return MakeRejected(ELRSaveOperationType::OverwriteManual, slotId,
			ELRSaveResultCode::RejectedProtectedSlot, TEXT("Automatic slot can only be written by AutoSave, CriticalSave, or NewGame."));
	}
	if (bPersistenceBlocked)
	{
		return MakeRejected(ELRSaveOperationType::OverwriteManual, slotId, ELRSaveResultCode::RejectedBusy,
			TEXT("Persistence is blocked until catalog recovery succeeds."));
	}
	if (!IsManualSaveAllowed())
	{
		return MakeRejected(ELRSaveOperationType::OverwriteManual, slotId, ELRSaveResultCode::RejectedNotEligible,
			TEXT("Manual save requires paused gameplay outside Memory."));
	}
	if (!slotId.IsValid() || !SaveCatalog || !SaveCatalog->FindSlot(slotId))
	{
		return MakeRejected(ELRSaveOperationType::OverwriteManual, slotId, ELRSaveResultCode::RejectedInvalidSlot,
			TEXT("Manual slot does not exist."));
	}
	FLRSaveDataV2 captured;
	FString error;
	if (!CaptureCurrentData(captured, error))
	{
		return MakeRejected(ELRSaveOperationType::OverwriteManual, slotId,
			ELRSaveResultCode::ProviderUnavailable, error);
	}
	return EnqueueOperation(ELRSaveOperationType::OverwriteManual, slotId, reasonId, &captured);
}

FLRSaveOperationResult ULRSaveSubsystem::RequestLoadSave(const FLRSaveSlotId slotId)
{
	if (bPersistenceBlocked)
	{
		return MakeRejected(ELRSaveOperationType::Load, slotId, ELRSaveResultCode::RejectedBusy,
			TEXT("Persistence is blocked until catalog recovery succeeds."));
	}
	if (!slotId.IsValid() || !SaveCatalog || !SaveCatalog->FindSlot(slotId))
	{
		return MakeRejected(ELRSaveOperationType::Load, slotId, ELRSaveResultCode::RejectedInvalidSlot,
			TEXT("Requested save slot does not exist."));
	}
	return EnqueueOperation(ELRSaveOperationType::Load, slotId, TEXT("Load"));
}

FLRSaveOperationResult ULRSaveSubsystem::RequestDeleteSave(const FLRSaveSlotId slotId)
{
	if (slotId.Type == ELRSaveSlotType::Auto)
	{
		return MakeRejected(ELRSaveOperationType::Delete, slotId, ELRSaveResultCode::RejectedProtectedSlot,
			TEXT("Automatic slot cannot be deleted."));
	}
	if (bPersistenceBlocked)
	{
		return MakeRejected(ELRSaveOperationType::Delete, slotId, ELRSaveResultCode::RejectedBusy,
			TEXT("Persistence is blocked until catalog recovery succeeds."));
	}
	if (!slotId.IsValid() || !SaveCatalog || !SaveCatalog->FindSlot(slotId))
	{
		return MakeRejected(ELRSaveOperationType::Delete, slotId, ELRSaveResultCode::RejectedInvalidSlot,
			TEXT("Requested save slot does not exist."));
	}
	return EnqueueOperation(ELRSaveOperationType::Delete, slotId, TEXT("Delete"));
}

FLRSaveOperationResult ULRSaveSubsystem::RequestContinue()
{
	if (bPersistenceBlocked)
	{
		return MakeRejected(ELRSaveOperationType::Continue, FLRSaveSlotId(), ELRSaveResultCode::RejectedBusy,
			TEXT("Persistence is blocked until catalog recovery succeeds."));
	}
	return EnqueueOperation(ELRSaveOperationType::Continue, FLRSaveSlotId(), TEXT("Continue"));
}

FLRSaveOperationResult ULRSaveSubsystem::RequestNewGame()
{
	if (bPersistenceBlocked)
	{
		return MakeRejected(ELRSaveOperationType::NewGame, MakeAutoSlotId(), ELRSaveResultCode::RejectedBusy,
			TEXT("Persistence is blocked until catalog recovery succeeds."));
	}
	const ULRGameInstanceSubsystem* data = GetGameInstance()
		? GetGameInstance()->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	const ULRGameContentSet* content = data ? data->GetContentSet() : nullptr;
	if (!content || content->NewGameMapId.IsNone())
	{
		return MakeRejected(ELRSaveOperationType::NewGame, MakeAutoSlotId(), ELRSaveResultCode::RejectedNotEligible,
			TEXT("New Game map is not configured."));
	}
	const FGuid operationId = FGuid::NewGuid();
	const FLRSaveOperationResult result = EnqueueOperation(ELRSaveOperationType::NewGame, MakeAutoSlotId(),
		TEXT("NewGame"), nullptr, ELRSaveMemoryPurpose::None, ELRSaveSlotHealth::Healthy, false, operationId);
	return result;
}
