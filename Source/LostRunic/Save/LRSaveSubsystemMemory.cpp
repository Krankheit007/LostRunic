#include "Save/LRSaveSubsystem.h"

#include "Core/LRGameplayTags.h"
#include "Core/LRLog.h"
#include "Framework/LRCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Narrative/LRDialogueSubsystem.h"
#include "Save/LRGameStatisticsSubsystem.h"
#include "Save/LRSaveRules.h"
#include "State/LRStateComponent.h"

namespace
{
	FLRSaveSlotId MakeMemoryAutoSlotId()
	{
		FLRSaveSlotId slotId;
		slotId.Type = ELRSaveSlotType::Auto;
		slotId.Guid = LRSaveV2Ids::AutoSlotGuid;
		return slotId;
	}
}

bool ULRSaveSubsystem::BeginDeathMemoryTransaction(ALRCharacter* character)
{
	if (bPersistenceBlocked || !character || !LRSaveRules::CanBeginMemoryTransaction(
		MemoryPhase, CurrentData.Player.ResumeAnchor))
	{
		UE_LOG(LogLostRunicSave, Warning, TEXT("Memory entry rejected phase=%d anchorValid=%d"),
			static_cast<int32>(MemoryPhase), CurrentData.Player.ResumeAnchor.IsValid() ? 1 : 0);
		return false;
	}
	FLRSaveDataV2 snapshot;
	FString error;
	if (!CaptureCurrentData(snapshot, error))
	{
		UE_LOG(LogLostRunicSave, Warning, TEXT("Memory snapshot capture failed: %s"), *error);
		return false;
	}
	if (ULRGameStatisticsSubsystem* statistics = GetGameInstance()
		? GetGameInstance()->GetSubsystem<ULRGameStatisticsSubsystem>() : nullptr)
	{
		statistics->RecordDeath();
		statistics->Capture(snapshot.Statistics);
	}
	else
	{
		UE_LOG(LogLostRunicSave, Warning, TEXT("Memory snapshot could not resolve statistics subsystem."));
		return false;
	}
	if (ULRDialogueSubsystem* dialogue = GetGameInstance()
		? GetGameInstance()->GetSubsystem<ULRDialogueSubsystem>() : nullptr)
	{
		dialogue->CaptureMemoryEventIds(snapshot.Story.MemoryEventIds);
	}
	HomeResumeSnapshot = snapshot;
	CurrentData = snapshot;
	bHasHomeResumeSnapshot = true;
	SetMemoryPhase(ELRMemoryTransactionPhase::AwaitingMemoryWorld);
	SetTransitionInput(true);
	if (TravelToMap(LRSaveIds::MemoryMapId))
	{
		return true;
	}
	bHasHomeResumeSnapshot = false;
	SetMemoryPhase(ELRMemoryTransactionPhase::None);
	SetTransitionInput(false);
	return false;
}

bool ULRSaveSubsystem::CommitMemoryEvent(const FName eventId)
{
	if (bPersistenceBlocked || MemoryPhase != ELRMemoryTransactionPhase::InMemory
		|| !bHasHomeResumeSnapshot || eventId.IsNone())
	{
		return false;
	}
	ULRDialogueSubsystem* dialogue = GetGameInstance()
		? GetGameInstance()->GetSubsystem<ULRDialogueSubsystem>() : nullptr;
	if (!dialogue || !dialogue->RecordMemoryEvent(eventId))
	{
		return false;
	}
	FLRSaveDataV2 captured = HomeResumeSnapshot;
	dialogue->CaptureMemoryEventIds(captured.Story.MemoryEventIds);
	HomeResumeSnapshot = captured;
	CurrentData = captured;
	const FLRSaveOperationResult result = EnqueueOperation(ELRSaveOperationType::CriticalSave,
		MakeMemoryAutoSlotId(), eventId, &captured, ELRSaveMemoryPurpose::Event);
	return result.Code == ELRSaveResultCode::Queued;
}

bool ULRSaveSubsystem::RequestReturnFromMemory()
{
	if (bPersistenceBlocked || MemoryPhase != ELRMemoryTransactionPhase::InMemory
		|| !bHasHomeResumeSnapshot || !HomeResumeSnapshot.Player.ResumeAnchor.IsValid())
	{
		return false;
	}
	SetMemoryPhase(ELRMemoryTransactionPhase::AwaitingResumeWorld);
	SetTransitionInput(true);
	if (TravelToMap(HomeResumeSnapshot.Player.ResumeAnchor.MapId))
	{
		return true;
	}
	SetMemoryPhase(ELRMemoryTransactionPhase::InMemory);
	SetTransitionInput(false);
	return false;
}

void ULRSaveSubsystem::HandleWorldReady(ALRCharacter* character)
{
	if (!character || !bHasHomeResumeSnapshot)
	{
		return;
	}
	if (MemoryPhase == ELRMemoryTransactionPhase::AwaitingMemoryWorld
		&& GetCurrentMapId() == LRSaveIds::MemoryMapId)
	{
		ApplyMemoryState(character);
		SetMemoryPhase(ELRMemoryTransactionPhase::SavingEntry);
		EnqueueOperation(ELRSaveOperationType::CriticalSave, MakeMemoryAutoSlotId(),
			LRSaveIds::MemoryEntryReason, &HomeResumeSnapshot, ELRSaveMemoryPurpose::Entry);
		return;
	}
	if (MemoryPhase != ELRMemoryTransactionPhase::AwaitingResumeWorld
		|| GetCurrentMapId() != HomeResumeSnapshot.Player.ResumeAnchor.MapId || !GetGameInstance())
	{
		return;
	}
	FString error;
	if (!LRSaveProviders::RestoreNonPlayer(SaveProviders, *GetGameInstance(), HomeResumeSnapshot, error)
		|| !LRSaveProviders::RestorePlayer(SaveProviders, *GetGameInstance(), HomeResumeSnapshot, error))
	{
		UE_LOG(LogLostRunicSave, Warning, TEXT("Memory resume restore failed: %s"), *error);
		SetMemoryPhase(ELRMemoryTransactionPhase::InMemory);
		SetTransitionInput(false);
		return;
	}
	FLRSaveDataV2 resumeSnapshot = HomeResumeSnapshot;
	if (ULRDialogueSubsystem* dialogue = GetGameInstance()->GetSubsystem<ULRDialogueSubsystem>())
	{
		dialogue->CaptureMemoryEventIds(resumeSnapshot.Story.MemoryEventIds);
	}
	if (ULRGameStatisticsSubsystem* statistics = GetGameInstance()->GetSubsystem<ULRGameStatisticsSubsystem>())
	{
		statistics->Capture(resumeSnapshot.Statistics);
	}
	HomeResumeSnapshot = resumeSnapshot;
	CurrentData = resumeSnapshot;
	SetMemoryPhase(ELRMemoryTransactionPhase::SavingReturn);
	EnqueueOperation(ELRSaveOperationType::CriticalSave, MakeMemoryAutoSlotId(),
		LRSaveIds::MemoryReturnReason, &resumeSnapshot, ELRSaveMemoryPurpose::Return);
}

void ULRSaveSubsystem::UpdateMemoryPhaseAfterOperation(const FLRQueuedSaveOperation& operation,
	const bool bSuccess)
{
	if (operation.Type != ELRSaveOperationType::CriticalSave)
	{
		return;
	}
	if (bSuccess)
	{
		CurrentData = operation.CapturedData;
	}
	if (operation.MemoryPurpose == ELRSaveMemoryPurpose::Entry)
	{
		SetMemoryPhase(ELRMemoryTransactionPhase::InMemory);
		SetTransitionInput(false);
	}
	else if (operation.MemoryPurpose == ELRSaveMemoryPurpose::Return)
	{
		if (bSuccess)
		{
			bHasHomeResumeSnapshot = false;
			SetMemoryPhase(ELRMemoryTransactionPhase::None);
		}
		else
		{
			SetMemoryPhase(ELRMemoryTransactionPhase::InMemory);
		}
		SetTransitionInput(false);
	}
}
