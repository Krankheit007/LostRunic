#include "Save/LRSaveSubsystem.h"

#include "Core/LRGameplayTags.h"
#include "Core/LRLog.h"
#include "Engine/World.h"
#include "Framework/LRCharacter.h"
#include "Items/LRInventoryComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Narrative/LRDialogueSubsystem.h"
#include "Save/LRSaveGame.h"
#include "Save/LRSaveRequestQueue.h"
#include "Save/LRSaveRules.h"
#include "State/LRStateComponent.h"
#include "TimerManager.h"

void ULRSaveSubsystem::CaptureRuntimeState()
{
	if (!WorkingSave)
	{
		return;
	}
	const ALRCharacter* character = Cast<ALRCharacter>(UGameplayStatics::GetPlayerCharacter(GetCurrentWorld(), 0));
	if (character)
	{
		character->GetInventoryComponent()->CaptureSaveState(WorkingSave->Inventory);
	}
	if (const ULRDialogueSubsystem* dialogueSubsystem = GetGameInstance()->GetSubsystem<ULRDialogueSubsystem>())
	{
		WorkingSave->Narrative.CompletedEventIds = dialogueSubsystem->GetCompletedEvents();
	}
}

void ULRSaveSubsystem::ApplyRuntimeState(ALRCharacter* character)
{
	if (!WorkingSave)
	{
		return;
	}
	if (character)
	{
		character->GetInventoryComponent()->RestoreSaveState(WorkingSave->Inventory);
		if (GetCurrentMapId() == WorkingSave->ResumeAnchor.MapId)
		{
			character->SetActorLocationAndRotation(WorkingSave->ResumeAnchor.Location,
				WorkingSave->ResumeAnchor.Rotation, false, nullptr, ETeleportType::TeleportPhysics);
		}
		if (ULRStateComponent* state = character->GetStateComponent(); state
			&& state->GetCurrentMode() == ELRPerceptionMode::Memory)
		{
			FLRStateChangeRequest request;
			request.TargetMode = ELRPerceptionMode::Normal;
			request.RequestType = ELRStateRequestType::Narrative;
			request.Source = LRGameplayTags::StateSourceNarrative;
			state->RequestStateChange(request);
		}
	}
	if (ULRDialogueSubsystem* dialogueSubsystem = GetGameInstance()->GetSubsystem<ULRDialogueSubsystem>())
	{
		dialogueSubsystem->RestoreCompletedEvents(WorkingSave->Narrative.CompletedEventIds);
	}
}

ULRSaveGame* ULRSaveSubsystem::CreateSnapshot() const
{
	return WorkingSave ? DuplicateObject<ULRSaveGame>(WorkingSave, const_cast<ULRSaveSubsystem*>(this)) : nullptr;
}

ELRSaveRequestResult ULRSaveSubsystem::QueueWrite(const FString& slotName, const FName reasonId,
	const ELRSaveWriteKind writeKind)
{
	if (!WorkingSave)
	{
		return ELRSaveRequestResult::MissingOrCorrupt;
	}
	CaptureRuntimeState();
	WorkingSave->LastSavedUtc = FDateTime::UtcNow();
	FLRQueuedSaveRequest request;
	request.Snapshot = CreateSnapshot();
	request.SlotName = slotName;
	request.ReasonId = reasonId.IsNone() ? LRSaveIds::AutoSlotReason : reasonId;
	request.Kind = writeKind;
	LRSaveRequestQueue::Enqueue(RequestQueue, MoveTemp(request));
	OnSaveWriteQueued.Broadcast(RequestQueue.Last().ReasonId, writeKind);
	UE_LOG(LogLostRunicSave, Log, TEXT("Save queued slot=%s reason=%s kind=%d pending=%d"), *slotName,
		*RequestQueue.Last().ReasonId.ToString(), static_cast<int32>(writeKind), RequestQueue.Num());
	StartNextWrite();
	return ELRSaveRequestResult::Queued;
}

void ULRSaveSubsystem::QueuePendingAutoSave()
{
	const FName reasonId = PendingAutoSaveReason.IsNone() ? LRSaveIds::AutoSlotReason : PendingAutoSaveReason;
	PendingAutoSaveReason = NAME_None;
	QueueWrite(LRSaveRules::MakeSlotName(ELRSaveSlotType::Auto), reasonId, ELRSaveWriteKind::Auto);
}

void ULRSaveSubsystem::StartNextWrite()
{
	if (bWriteInProgress || RequestQueue.IsEmpty())
	{
		return;
	}
	LRSaveRequestQueue::Dequeue(RequestQueue, ActiveRequest);
	bWriteInProgress = true;
	StartActiveWrite();
}

void ULRSaveSubsystem::StartActiveWrite()
{
	if (!ActiveRequest.Snapshot)
	{
		CompleteActiveWrite(false);
		return;
	}
	FAsyncSaveGameToSlotDelegate saveDelegate;
	saveDelegate.BindUObject(this, &ULRSaveSubsystem::HandleAsyncSaveFinished);
	UGameplayStatics::AsyncSaveGameToSlot(ActiveRequest.Snapshot, ActiveRequest.SlotName, 0, saveDelegate);
}

void ULRSaveSubsystem::RetryActiveWrite()
{
	if (bWriteInProgress)
	{
		StartActiveWrite();
	}
}

void ULRSaveSubsystem::HandleAsyncSaveFinished(const FString& slotName, const int32 userIndex, const bool bSuccess)
{
	CompleteActiveWrite(bSuccess);
}

void ULRSaveSubsystem::CompleteActiveWrite(const bool bSuccess)
{
	if (!bWriteInProgress)
	{
		return;
	}
	if (!bSuccess && ActiveRequest.RetryAttempt < GetEffectiveTuning().RetryCount)
	{
		++ActiveRequest.RetryAttempt;
		UWorld* world = GetCurrentWorld();
		if (world)
		{
			world->GetTimerManager().SetTimer(RetryTimer, this, &ULRSaveSubsystem::RetryActiveWrite,
				GetEffectiveTuning().RetryDelaySeconds, false);
			return;
		}
	}

	const FName reasonId = ActiveRequest.ReasonId;
	const ELRSaveWriteKind writeKind = ActiveRequest.Kind;
	if (bSuccess)
	{
		UE_LOG(LogLostRunicSave, Log, TEXT("Save completed slot=%s reason=%s kind=%d retries=%d"),
			*ActiveRequest.SlotName, *reasonId.ToString(), static_cast<int32>(writeKind), ActiveRequest.RetryAttempt);
	}
	else
	{
		UE_LOG(LogLostRunicSave, Warning, TEXT("Save failed slot=%s reason=%s kind=%d retries=%d"),
			*ActiveRequest.SlotName, *reasonId.ToString(), static_cast<int32>(writeKind), ActiveRequest.RetryAttempt);
	}
	OnSaveWriteCompleted.Broadcast(reasonId, bSuccess);
	UpdateMemoryPhaseAfterWrite(writeKind, bSuccess);
	ActiveRequest = FLRQueuedSaveRequest();
	bWriteInProgress = false;
	StartNextWrite();
}

ELRSaveRequestResult ULRSaveSubsystem::LoadSlot(const FString& slotName)
{
	if (!UGameplayStatics::DoesSaveGameExist(slotName, 0))
	{
		OnSaveLoadCompleted.Broadcast(slotName, false, TEXT("Save does not exist."));
		return ELRSaveRequestResult::MissingOrCorrupt;
	}
	ULRSaveGame* loadedSave = Cast<ULRSaveGame>(UGameplayStatics::LoadGameFromSlot(slotName, 0));
	FString error;
	if (!loadedSave || !loadedSave->MigrateToLatest(error))
	{
		const FString failure = error.IsEmpty() ? TEXT("Save is corrupt or uses an unsupported class.") : error;
		UE_LOG(LogLostRunicSave, Warning, TEXT("Save load failed slot=%s error=%s"), *slotName, *failure);
		OnSaveLoadCompleted.Broadcast(slotName, false, failure);
		return ELRSaveRequestResult::MissingOrCorrupt;
	}
	WorkingSave = loadedSave;
	bAwaitingLoadedResume = false;
	const FName currentMapId = GetCurrentMapId();
	if (WorkingSave->ResumeAnchor.IsValid() && currentMapId != WorkingSave->ResumeAnchor.MapId)
	{
		if (!TravelToMap(WorkingSave->ResumeAnchor.MapId))
		{
			const FString failure = FString::Printf(TEXT("Resume map '%s' is unavailable."),
				*WorkingSave->ResumeAnchor.MapId.ToString());
			UE_LOG(LogLostRunicSave, Warning, TEXT("Save load rejected slot=%s error=%s"), *slotName, *failure);
			OnSaveLoadCompleted.Broadcast(slotName, false, failure);
			return ELRSaveRequestResult::RejectedUnavailableMap;
		}
		bAwaitingLoadedResume = true;
	}
	else
	{
		ApplyRuntimeState(Cast<ALRCharacter>(UGameplayStatics::GetPlayerCharacter(GetCurrentWorld(), 0)));
	}
	OnSaveLoadCompleted.Broadcast(slotName, true, FString());
	return ELRSaveRequestResult::Loaded;
}

ELRSaveRequestResult ULRSaveSubsystem::LoadManualSlot(const int32 manualSlotIndex)
{
	return LRSaveRules::IsManualSlotValid(manualSlotIndex, GetManualSlotCount())
		? LoadSlot(LRSaveRules::MakeSlotName(ELRSaveSlotType::Manual, manualSlotIndex))
		: ELRSaveRequestResult::RejectedInvalidSlot;
}

ELRSaveRequestResult ULRSaveSubsystem::ContinueLatestSave()
{
	FString latestSlot;
	FDateTime latestTime;
	const auto considerSlot = [this, &latestSlot, &latestTime](const FString& slotName)
	{
		ULRSaveGame* save = Cast<ULRSaveGame>(UGameplayStatics::LoadGameFromSlot(slotName, 0));
		if (save && save->LastSavedUtc > latestTime)
		{
			latestTime = save->LastSavedUtc;
			latestSlot = slotName;
		}
	};
	const FString autoSlot = LRSaveRules::MakeSlotName(ELRSaveSlotType::Auto);
	if (UGameplayStatics::DoesSaveGameExist(autoSlot, 0))
	{
		considerSlot(autoSlot);
	}
	for (int32 slotIndex = 0; slotIndex < GetManualSlotCount(); ++slotIndex)
	{
		const FString slotName = LRSaveRules::MakeSlotName(ELRSaveSlotType::Manual, slotIndex);
		if (UGameplayStatics::DoesSaveGameExist(slotName, 0))
		{
			considerSlot(slotName);
		}
	}
	return latestSlot.IsEmpty() ? ELRSaveRequestResult::MissingOrCorrupt : LoadSlot(latestSlot);
}
