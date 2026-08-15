#include "Save/LRSaveSubsystem.h"

#include "Core/LRLog.h"
#include "Data/LRGameContentSet.h"
#include "Framework/LRCharacter.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Save/LRSaveAnchor.h"
#include "Save/LRSaveCatalog.h"
#include "Save/LRSaveCatalogStore.h"
#include "Save/LRSavePayload.h"
#include "Save/LRSaveProvider.h"

void ULRSaveSubsystem::StartLoad()
{
	OperationState = ELRSaveOperationState::ReadingPayload;
	TArray<FLRSaveSlotMetadata> candidates;
	if (ActiveOperation.Type == ELRSaveOperationType::Continue)
	{
		candidates = SaveCatalog->Slots;
		candidates.Sort([](const FLRSaveSlotMetadata& left, const FLRSaveSlotMetadata& right)
		{
			return left.SavedAtUtc == right.SavedAtUtc
				? left.SaveSequence > right.SaveSequence : left.SavedAtUtc > right.SavedAtUtc;
		});
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
			OperationState = ELRSaveOperationState::AwaitingWorld;
			OnSaveLoadRequested.Broadcast(ActiveOperation.OperationId,
				ActivePayload->Data.Player.CurrentMapId);
			return;
		}
		if (FLRSaveCatalogStore::IsDeterministicHealth(health))
		{
			EnqueueHealthRepair(metadata.SlotId, health);
		}
	}
	CompleteOperation(lastCode, lastError);
}

void ULRSaveSubsystem::NotifyLoadWorldReady(const FGuid operationId)
{
	if (OperationState != ELRSaveOperationState::AwaitingWorld
		|| ActiveOperation.OperationId != operationId || !ActivePayload || !GetGameInstance())
	{
		return;
	}
	OperationState = ELRSaveOperationState::Restoring;
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

void ULRSaveSubsystem::NotifyLoadPreparationFailed(const FGuid operationId, const FString& diagnostic)
{
	if (OperationState == ELRSaveOperationState::AwaitingWorld
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
	PendingNewGameOperationId = ActiveOperation.OperationId;
	OperationState = ELRSaveOperationState::AwaitingWorld;
	OnSaveNewGameRequested.Broadcast(ActiveOperation.OperationId, content->NewGameMapId);
}

void ULRSaveSubsystem::NotifyNewGameWorldReady(const FGuid operationId)
{
	if (OperationState != ELRSaveOperationState::AwaitingWorld
		|| PendingNewGameOperationId != operationId || !GetGameInstance())
	{
		return;
	}
	const ULRGameInstanceSubsystem* dataSubsystem = GetGameInstance()->GetSubsystem<ULRGameInstanceSubsystem>();
	const ULRGameContentSet* content = dataSubsystem ? dataSubsystem->GetContentSet() : nullptr;
	const FLRMapRegistration* map = content ? content->FindMapRegistration(content->NewGameMapId) : nullptr;
	ALRCharacter* character = Cast<ALRCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	ALRSaveAnchor* anchor = map && character
		? ALRSaveAnchor::FindById(character->GetWorld(), map->DefaultStartAnchorId) : nullptr;
	if (!anchor || !character->SetActorLocationAndRotation(anchor->GetActorLocation(), anchor->GetActorRotation(),
		false, nullptr, ETeleportType::TeleportPhysics))
	{
		PendingNewGameOperationId.Invalidate();
		CompleteOperation(ELRSaveResultCode::ProviderUnavailable,
			TEXT("New Game default start anchor is unavailable."));
		return;
	}
	FString error;
	if (!LRSaveProviders::ResetForNewGame(SaveProviders, *GetGameInstance(), error)
		|| !CaptureCurrentData(ActiveOperation.CapturedData, error))
	{
		PendingNewGameOperationId.Invalidate();
		CompleteOperation(ELRSaveResultCode::ProviderUnavailable, error);
		return;
	}
	ActiveOperation.bHasCapturedData = true;
	CurrentData = ActiveOperation.CapturedData;
	PendingNewGameOperationId.Invalidate();
	StartWrite();
}

void ULRSaveSubsystem::ApplyDataToRuntime(const FLRSaveDataV2& data, ALRCharacter* character)
{
	if (!GetGameInstance())
	{
		return;
	}
	FString error;
	if (!LRSaveProviders::RestoreNonPlayer(SaveProviders, *GetGameInstance(), data, error))
	{
		UE_LOG(LogLostRunicSave, Warning, TEXT("Runtime restore failed: %s"), *error);
		return;
	}
	if (character && !LRSaveProviders::RestorePlayer(SaveProviders, *GetGameInstance(), data, error))
	{
		UE_LOG(LogLostRunicSave, Warning, TEXT("Player runtime restore failed: %s"), *error);
	}
}
