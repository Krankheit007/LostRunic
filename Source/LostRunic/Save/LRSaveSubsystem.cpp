#include "Save/LRSaveSubsystem.h"

#include "Core/LRGameplayTags.h"
#include "Core/LRLog.h"
#include "Data/LRGameContentSet.h"
#include "Data/LRGameTuningSet.h"
#include "Data/LRSaveTuning.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/LRCharacter.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "Framework/LRPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Narrative/LRDialogueSubsystem.h"
#include "Save/LRSaveCatalog.h"
#include "Save/LRSaveCatalogStore.h"
#include "Save/LRSaveProvider.h"
#include "Save/LRSaveRules.h"
#include "State/LRStateComponent.h"
#include "UI/LRHUD.h"
#include "UI/LRPlayerUIComponent.h"

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
}

void ULRSaveSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
	Super::Initialize(collection);
	collection.InitializeDependency<ULRGameInstanceSubsystem>();
	collection.InitializeDependency<ULRDialogueSubsystem>();

	const ULRGameInstanceSubsystem* dataSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	Tuning = dataSubsystem && dataSubsystem->GetTuningSet() ? dataSubsystem->GetTuningSet()->Save : nullptr;
	FLRCatalogRecoveryResult recovery;
	SaveCatalog = FLRSaveCatalogStore::LoadBestCatalog(this, recovery);
	LRSaveProviders::CreateRequired(SaveProviders);
	if (!recovery.Diagnostic.IsEmpty())
	{
		UE_LOG(LogLostRunicSave, Log, TEXT("V2 catalog bootstrap: %s"), *recovery.Diagnostic);
	}
	if (SaveCatalog && SaveCatalog->PendingOperation.IsSet())
	{
		EnqueuePendingCatalogRepair();
	}
	if (ULRDialogueSubsystem* dialogue = GetGameInstance()
		? GetGameInstance()->GetSubsystem<ULRDialogueSubsystem>() : nullptr)
	{
		dialogue->OnEventCommitted.AddDynamic(this, &ULRSaveSubsystem::HandleNarrativeEventCommitted);
	}
}

void ULRSaveSubsystem::Deinitialize()
{
	if (ULRDialogueSubsystem* dialogue = GetGameInstance()
		? GetGameInstance()->GetSubsystem<ULRDialogueSubsystem>() : nullptr)
	{
		dialogue->OnEventCommitted.RemoveDynamic(this, &ULRSaveSubsystem::HandleNarrativeEventCommitted);
	}
	if (UWorld* world = GetCurrentWorld())
	{
		FTimerManager& timers = world->GetTimerManager();
		timers.ClearTimer(AutoSaveDebounceTimer);
		timers.ClearTimer(ExplicitRetryTimer);
		timers.ClearTimer(OperationTimeoutTimer);
		timers.ClearTimer(AsyncWatchdogTimer);
	}
	OperationQueue.Reset();
	ActiveOperation = FLRQueuedSaveOperation();
	ActivePayload = nullptr;
	SaveCatalog = nullptr;
	SaveProviders.Reset();
	OperationState = ELRSaveOperationState::Idle;
	PendingAutoSaveOperationId.Invalidate();
	PendingNewGameOperationId.Invalidate();
	Tuning = nullptr;
	Super::Deinitialize();
}

TArray<FLRSaveSlotMetadata> ULRSaveSubsystem::GetSaveSlots() const
{
	return SaveCatalog ? SaveCatalog->Slots : TArray<FLRSaveSlotMetadata>();
}

int32 ULRSaveSubsystem::GetMaxManualSaveSlots() const
{
	return GetManualSlotCount();
}

FLRSaveOperationResult ULRSaveSubsystem::MakeRejected(const ELRSaveOperationType type,
	const FLRSaveSlotId& slotId, const ELRSaveResultCode code, const FString& diagnostic) const
{
	return MakeOperationResult(FGuid::NewGuid(), type, slotId, code, diagnostic);
}

bool ULRSaveSubsystem::CaptureCurrentData(FLRSaveDataV2& outData, FString& outError)
{
	UGameInstance* gameInstance = GetGameInstance();
	if (!gameInstance)
	{
		outError = TEXT("GameInstance is unavailable.");
		return false;
	}
	FLRSaveDataV2 captured;
	if (!LRSaveProviders::CaptureAll(SaveProviders, *gameInstance, captured, outError))
	{
		return false;
	}
	outData = captured;
	CurrentData = captured;
	return true;
}

void ULRSaveSubsystem::SetResumeAnchor(const FLRResumeAnchor& anchor)
{
	if (!anchor.IsValid())
	{
		UE_LOG(LogLostRunicSave, Warning, TEXT("Rejected invalid resume anchor map=%s anchor=%s."),
			*anchor.MapId.ToString(), *anchor.AnchorId.ToString());
		return;
	}
	CurrentData.Player.ResumeAnchor = anchor;
}

FLRResumeAnchor ULRSaveSubsystem::GetResumeAnchor() const
{
	return CurrentData.Player.ResumeAnchor;
}

bool ULRSaveSubsystem::IsManualSaveAllowed() const
{
	const UWorld* world = GetCurrentWorld();
	return !bPersistenceBlocked && LRSaveRules::IsManualSaveAllowed(MemoryPhase, world && world->IsPaused());
}

void ULRSaveSubsystem::HandleNarrativeEventCommitted(const FName eventId, const ELRSavePolicy savePolicy)
{
	if (savePolicy == ELRSavePolicy::AutoOnComplete)
	{
		RequestAutoSave(eventId);
		return;
	}
	if (savePolicy != ELRSavePolicy::Critical || bPersistenceBlocked)
	{
		return;
	}
	FLRSaveDataV2 captured;
	FString error;
	FLRSaveSlotId autoSlot;
	autoSlot.Type = ELRSaveSlotType::Auto;
	autoSlot.Guid = LRSaveV2Ids::AutoSlotGuid;
	if (CaptureCurrentData(captured, error))
	{
		EnqueueOperation(ELRSaveOperationType::CriticalSave, autoSlot, eventId, &captured);
	}
	else
	{
		UE_LOG(LogLostRunicSave, Warning, TEXT("Critical narrative capture failed event=%s error=%s"),
			*eventId.ToString(), *error);
	}
}

FLRSaveOperationResult ULRSaveSubsystem::RequestAutoSave(const FName reasonId)
{
	if (bPersistenceBlocked)
	{
		FLRSaveSlotId autoSlot;
		autoSlot.Type = ELRSaveSlotType::Auto;
		autoSlot.Guid = LRSaveV2Ids::AutoSlotGuid;
		return MakeRejected(ELRSaveOperationType::AutoSave, autoSlot, ELRSaveResultCode::RejectedBusy,
			TEXT("Persistence is blocked until catalog recovery succeeds."));
	}

	FLRSaveSlotId autoSlot;
	autoSlot.Type = ELRSaveSlotType::Auto;
	autoSlot.Guid = LRSaveV2Ids::AutoSlotGuid;
	const FGuid operationId = FGuid::NewGuid();
	const FName effectiveReason = reasonId.IsNone() ? LRSaveIds::AutoSlotReason : reasonId;
	if (GetEffectiveTuning().AutoSaveDebounceSeconds > 0.0f)
	{
		if (UWorld* world = GetCurrentWorld())
		{
			PendingAutoSaveReason = effectiveReason;
			PendingAutoSaveOperationId = operationId;
			FTimerDelegate callback = FTimerDelegate::CreateWeakLambda(this,
				[this]() { CapturePendingAutoSave(); });
			world->GetTimerManager().SetTimer(AutoSaveDebounceTimer, callback,
				GetEffectiveTuning().AutoSaveDebounceSeconds, false);
			return MakeOperationResult(operationId, ELRSaveOperationType::AutoSave, autoSlot,
				ELRSaveResultCode::Queued, TEXT("Automatic save is waiting for debounce."));
		}
	}

	FLRSaveDataV2 captured;
	FString error;
	if (!CaptureCurrentData(captured, error))
	{
		return MakeOperationResult(operationId, ELRSaveOperationType::AutoSave, autoSlot,
			ELRSaveResultCode::ProviderUnavailable, error);
	}
	return EnqueueOperation(ELRSaveOperationType::AutoSave, autoSlot, effectiveReason, &captured,
		ELRSaveMemoryPurpose::None, ELRSaveSlotHealth::Healthy, false, operationId);
}

void ULRSaveSubsystem::CapturePendingAutoSave()
{
	const FGuid operationId = PendingAutoSaveOperationId;
	const FName reasonId = PendingAutoSaveReason;
	PendingAutoSaveOperationId.Invalidate();
	PendingAutoSaveReason = NAME_None;
	if (!operationId.IsValid() || bPersistenceBlocked)
	{
		return;
	}
	FLRSaveDataV2 captured;
	FString error;
	FLRSaveSlotId autoSlot;
	autoSlot.Type = ELRSaveSlotType::Auto;
	autoSlot.Guid = LRSaveV2Ids::AutoSlotGuid;
	if (!CaptureCurrentData(captured, error))
	{
		OnSaveOperationCompleted.Broadcast(MakeOperationResult(operationId, ELRSaveOperationType::AutoSave,
			autoSlot, ELRSaveResultCode::ProviderUnavailable, error));
		return;
	}
	EnqueueOperation(ELRSaveOperationType::AutoSave, autoSlot, reasonId, &captured,
		ELRSaveMemoryPurpose::None, ELRSaveSlotHealth::Healthy, false, operationId);
}

const ULRSaveTuning& ULRSaveSubsystem::GetEffectiveTuning() const
{
	return Tuning ? *Tuning : *GetDefault<ULRSaveTuning>();
}

int32 ULRSaveSubsystem::GetManualSlotCount() const
{
	return GetEffectiveTuning().MaxManualSaveSlots;
}

UWorld* ULRSaveSubsystem::GetCurrentWorld() const
{
	return GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
}

FName ULRSaveSubsystem::GetCurrentMapId() const
{
	const ULRGameInstanceSubsystem* data = GetGameInstance()
		? GetGameInstance()->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	const ULRGameContentSet* content = data ? data->GetContentSet() : nullptr;
	return content ? content->FindMapIdForWorld(GetCurrentWorld()) : NAME_None;
}

bool ULRSaveSubsystem::TravelToMap(const FName mapId)
{
	const ULRGameInstanceSubsystem* data = GetGameInstance()
		? GetGameInstance()->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	const ULRGameContentSet* content = data ? data->GetContentSet() : nullptr;
	const TSoftObjectPtr<UWorld> map = content ? content->FindMap(mapId) : TSoftObjectPtr<UWorld>();
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
	UE_LOG(LogLostRunicSave, Log, TEXT("Memory phase=%d"), static_cast<int32>(MemoryPhase));
}

void ULRSaveSubsystem::SetTransitionInput(const bool bVisible) const
{
	ALRPlayerController* controller = Cast<ALRPlayerController>(
		UGameplayStatics::GetPlayerController(GetCurrentWorld(), 0));
	if (!controller)
	{
		return;
	}
	if (ALRHUD* hud = controller->GetHUD<ALRHUD>())
	{
		hud->ShowTransition(bVisible);
	}
	if (ULRPlayerUIComponent* playerUi = controller->GetPlayerUI())
	{
		playerUi->SetTransitionLayer(bVisible);
	}
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
