#include "Save/LRSaveSubsystem.h"

#include "Core/LRLog.h"
#include "Data/LRGameTuningSet.h"
#include "Data/LRSaveTuning.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "Narrative/LRDialogueSubsystem.h"
#include "Save/LRSaveGame.h"
#include "Save/LRSaveRules.h"
#include "TimerManager.h"

void ULRSaveSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
	Super::Initialize(collection);
	collection.InitializeDependency<ULRGameInstanceSubsystem>();
	collection.InitializeDependency<ULRDialogueSubsystem>();
	const ULRGameInstanceSubsystem* dataSubsystem = GetGameInstance()->GetSubsystem<ULRGameInstanceSubsystem>();
	Tuning = dataSubsystem && dataSubsystem->GetTuningSet() ? dataSubsystem->GetTuningSet()->Save : nullptr;
	WorkingSave = NewObject<ULRSaveGame>(this);
	if (ULRDialogueSubsystem* dialogueSubsystem = GetGameInstance()->GetSubsystem<ULRDialogueSubsystem>())
	{
		dialogueSubsystem->OnEventCommitted.AddDynamic(this, &ULRSaveSubsystem::HandleNarrativeEventCommitted);
	}
}

void ULRSaveSubsystem::Deinitialize()
{
	if (ULRDialogueSubsystem* dialogueSubsystem = GetGameInstance()->GetSubsystem<ULRDialogueSubsystem>())
	{
		dialogueSubsystem->OnEventCommitted.RemoveDynamic(this, &ULRSaveSubsystem::HandleNarrativeEventCommitted);
	}
	if (UWorld* world = GetCurrentWorld())
	{
		world->GetTimerManager().ClearTimer(AutoSaveDebounceTimer);
		world->GetTimerManager().ClearTimer(RetryTimer);
	}
	RequestQueue.Reset();
	ActiveRequest = FLRQueuedSaveRequest();
	WorkingSave = nullptr;
	Tuning = nullptr;
	bAwaitingLoadedResume = false;
	Super::Deinitialize();
}

void ULRSaveSubsystem::SetResumeAnchor(const FLRResumeAnchor& anchor)
{
	if (!anchor.IsValid())
	{
		UE_LOG(LogLostRunicSave, Warning, TEXT("SaveSubsystem rejected invalid resume anchor map=%s anchor=%s."),
			*anchor.MapId.ToString(), *anchor.AnchorId.ToString());
		return;
	}
	WorkingSave->ResumeAnchor = anchor;
}

FLRResumeAnchor ULRSaveSubsystem::GetResumeAnchor() const
{
	return WorkingSave ? WorkingSave->ResumeAnchor : FLRResumeAnchor();
}

ELRSaveRequestResult ULRSaveSubsystem::RequestAutoSave(const FName reasonId)
{
	if (!WorkingSave)
	{
		return ELRSaveRequestResult::MissingOrCorrupt;
	}
	PendingAutoSaveReason = reasonId.IsNone() ? LRSaveIds::AutoSlotReason : reasonId;
	UWorld* world = GetCurrentWorld();
	if (!world || GetEffectiveTuning().AutoSaveDebounceSeconds <= 0.0f)
	{
		QueuePendingAutoSave();
		return ELRSaveRequestResult::Queued;
	}
	world->GetTimerManager().SetTimer(AutoSaveDebounceTimer, this, &ULRSaveSubsystem::QueuePendingAutoSave,
		GetEffectiveTuning().AutoSaveDebounceSeconds, false);
	return ELRSaveRequestResult::Scheduled;
}

ELRSaveRequestResult ULRSaveSubsystem::RequestManualSave(const int32 manualSlotIndex, const FName reasonId)
{
	if (!LRSaveRules::IsManualSaveAllowed(MemoryPhase))
	{
		return ELRSaveRequestResult::RejectedMemoryManual;
	}
	if (!LRSaveRules::IsManualSlotValid(manualSlotIndex, GetManualSlotCount()))
	{
		return ELRSaveRequestResult::RejectedInvalidSlot;
	}
	return QueueWrite(LRSaveRules::MakeSlotName(ELRSaveSlotType::Manual, manualSlotIndex), reasonId, ELRSaveWriteKind::Manual);
}

ELRSaveRequestResult ULRSaveSubsystem::RequestCriticalSave(const FName reasonId)
{
	return QueueWrite(LRSaveRules::MakeSlotName(ELRSaveSlotType::Auto), reasonId, ELRSaveWriteKind::Critical);
}

bool ULRSaveSubsystem::IsManualSaveAllowed() const
{
	return LRSaveRules::IsManualSaveAllowed(MemoryPhase);
}

void ULRSaveSubsystem::HandleNarrativeEventCommitted(const FName eventId, const ELRSavePolicy savePolicy)
{
	if (savePolicy == ELRSavePolicy::AutoOnComplete)
	{
		RequestAutoSave(eventId);
	}
	else if (savePolicy == ELRSavePolicy::Critical)
	{
		RequestCriticalSave(eventId);
	}
}

const ULRSaveTuning& ULRSaveSubsystem::GetEffectiveTuning() const
{
	return Tuning ? *Tuning : *GetDefault<ULRSaveTuning>();
}

int32 ULRSaveSubsystem::GetManualSlotCount() const
{
	return GetEffectiveTuning().ManualSlotCount;
}

UWorld* ULRSaveSubsystem::GetCurrentWorld() const
{
	return GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
}
