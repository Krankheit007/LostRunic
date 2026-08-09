#include "Save/LRSaveSubsystem.h"

#include "Core/LRGameplayTags.h"
#include "Core/LRLog.h"
#include "Data/LRGameContentSet.h"
#include "Framework/LRCharacter.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "Framework/LRPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Save/LRSaveGame.h"
#include "Save/LRSaveRules.h"
#include "State/LRStateComponent.h"
#include "UI/LRHUD.h"

bool ULRSaveSubsystem::BeginDeathMemoryTransaction(ALRCharacter* character)
{
	if (!WorkingSave || !LRSaveRules::CanBeginMemoryTransaction(MemoryPhase, WorkingSave->ResumeAnchor))
	{
		UE_LOG(LogLostRunicSave, Warning, TEXT("Memory transaction rejected phase=%d anchor=%s"),
			static_cast<int32>(MemoryPhase), WorkingSave && WorkingSave->ResumeAnchor.IsValid() ? TEXT("valid") : TEXT("invalid"));
		return false;
	}
	CaptureRuntimeState();
	++WorkingSave->Narrative.DeathCount;
	SetMemoryPhase(ELRMemoryTransactionPhase::AwaitingMemoryWorld);
	SetTransitionInput(true);
	if (TravelToMap(LRSaveIds::MemoryMapId))
	{
		return true;
	}
	SetMemoryPhase(ELRMemoryTransactionPhase::None);
	SetTransitionInput(false);
	return false;
}

bool ULRSaveSubsystem::CommitMemoryEvent(const FName eventId)
{
	if (MemoryPhase != ELRMemoryTransactionPhase::InMemory || eventId.IsNone() || !WorkingSave)
	{
		return false;
	}
	if (WorkingSave->Narrative.MemoryEventIds.Contains(eventId))
	{
		return false;
	}
	WorkingSave->Narrative.MemoryEventIds.Add(eventId);
	return QueueWrite(LRSaveRules::MakeSlotName(ELRSaveSlotType::Auto), eventId, ELRSaveWriteKind::MemoryEvent)
		== ELRSaveRequestResult::Queued;
}

bool ULRSaveSubsystem::RequestReturnFromMemory()
{
	if (!WorkingSave || MemoryPhase != ELRMemoryTransactionPhase::InMemory || !WorkingSave->ResumeAnchor.IsValid())
	{
		return false;
	}
	SetMemoryPhase(ELRMemoryTransactionPhase::AwaitingResumeWorld);
	SetTransitionInput(true);
	if (TravelToMap(WorkingSave->ResumeAnchor.MapId))
	{
		return true;
	}
	SetMemoryPhase(ELRMemoryTransactionPhase::InMemory);
	SetTransitionInput(false);
	return false;
}

void ULRSaveSubsystem::HandleWorldReady(ALRCharacter* character)
{
	const FName currentMapId = GetCurrentMapId();
	if (bAwaitingLoadedResume && WorkingSave && currentMapId == WorkingSave->ResumeAnchor.MapId)
	{
		bAwaitingLoadedResume = false;
		ApplyRuntimeState(character);
		return;
	}
	if (LRSaveRules::IsMemoryEntryWorld(MemoryPhase, currentMapId))
	{
		ApplyMemoryState(character);
		SetMemoryPhase(ELRMemoryTransactionPhase::SavingEntry);
		QueueWrite(LRSaveRules::MakeSlotName(ELRSaveSlotType::Auto), LRSaveIds::MemoryEntryReason,
			ELRSaveWriteKind::MemoryEntry);
		return;
	}
	if (WorkingSave && LRSaveRules::IsResumeWorld(MemoryPhase, currentMapId, WorkingSave->ResumeAnchor))
	{
		ApplyRuntimeState(character);
		SetMemoryPhase(ELRMemoryTransactionPhase::SavingReturn);
		QueueWrite(LRSaveRules::MakeSlotName(ELRSaveSlotType::Auto), LRSaveIds::MemoryReturnReason,
			ELRSaveWriteKind::MemoryReturn);
	}
}

void ULRSaveSubsystem::UpdateMemoryPhaseAfterWrite(const ELRSaveWriteKind writeKind, const bool bSuccess)
{
	ELRMemoryTransactionPhase nextPhase = LRSaveRules::ResolveAfterWrite(MemoryPhase, writeKind, bSuccess);
	if (!bSuccess && writeKind == ELRSaveWriteKind::MemoryEntry)
	{
		nextPhase = ELRMemoryTransactionPhase::InMemory;
	}
	if (!bSuccess && writeKind == ELRSaveWriteKind::MemoryReturn)
	{
		nextPhase = ELRMemoryTransactionPhase::None;
	}
	if (nextPhase == MemoryPhase)
	{
		return;
	}
	SetMemoryPhase(nextPhase);
	if (nextPhase == ELRMemoryTransactionPhase::InMemory || nextPhase == ELRMemoryTransactionPhase::None)
	{
		SetTransitionInput(false);
	}
}

FName ULRSaveSubsystem::GetCurrentMapId() const
{
	const ULRGameInstanceSubsystem* dataSubsystem = GetGameInstance()->GetSubsystem<ULRGameInstanceSubsystem>();
	const ULRGameContentSet* contentSet = dataSubsystem ? dataSubsystem->GetContentSet() : nullptr;
	return contentSet ? contentSet->FindMapIdForWorld(GetCurrentWorld()) : NAME_None;
}

bool ULRSaveSubsystem::TravelToMap(const FName mapId)
{
	const ULRGameInstanceSubsystem* dataSubsystem = GetGameInstance()->GetSubsystem<ULRGameInstanceSubsystem>();
	const ULRGameContentSet* contentSet = dataSubsystem ? dataSubsystem->GetContentSet() : nullptr;
	const TSoftObjectPtr<UWorld> map = contentSet ? contentSet->FindMap(mapId) : TSoftObjectPtr<UWorld>();
	if (map.IsNull())
	{
		UE_LOG(LogLostRunicSave, Warning, TEXT("Save travel rejected map=%s is not registered."), *mapId.ToString());
		return false;
	}
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, map);
	return true;
}

void ULRSaveSubsystem::SetMemoryPhase(const ELRMemoryTransactionPhase newPhase)
{
	if (MemoryPhase == newPhase)
	{
		return;
	}
	MemoryPhase = newPhase;
	OnMemoryTransactionChanged.Broadcast(MemoryPhase);
	UE_LOG(LogLostRunicSave, Log, TEXT("Memory transaction phase=%d"), static_cast<int32>(MemoryPhase));
}

void ULRSaveSubsystem::SetTransitionInput(const bool bVisible) const
{
	ALRPlayerController* controller = Cast<ALRPlayerController>(UGameplayStatics::GetPlayerController(GetCurrentWorld(), 0));
	if (!controller)
	{
		return;
	}
	if (ALRHUD* hud = controller->GetHUD<ALRHUD>())
	{
		hud->ShowTransition(bVisible);
	}
	controller->SetLRInputMode(bVisible ? ELRInputMode::Transition : ELRInputMode::Gameplay);
}

void ULRSaveSubsystem::ApplyMemoryState(ALRCharacter* character) const
{
	ULRStateComponent* state = character ? character->GetStateComponent() : nullptr;
	if (!state || state->GetCurrentMode() == ELRPerceptionMode::Memory)
	{
		return;
	}
	FLRStateChangeRequest request;
	request.TargetMode = ELRPerceptionMode::Memory;
	request.RequestType = ELRStateRequestType::Death;
	request.Source = LRGameplayTags::StateSourceDeath;
	state->RequestStateChange(request);
}
