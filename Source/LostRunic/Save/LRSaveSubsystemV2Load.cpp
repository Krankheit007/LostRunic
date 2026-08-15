#include "Save/LRSaveSubsystem.h"

#include "Engine/GameInstance.h"
#include "Data/LRGameContentSet.h"
#include "Framework/LRCharacter.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Save/LRSaveCatalog.h"
#include "Save/LRSaveCatalogStore.h"
#include "Save/LRSavePayload.h"
#include "Save/LRSaveProvider.h"
#include "Save/LRSaveAnchor.h"

bool ULRSaveSubsystem::PersistDeterministicHealth(const FLRSaveSlotId& slotId,
	const ELRSaveSlotHealth health)
{
	if (!SaveCatalog || !FLRSaveCatalogStore::IsDeterministicHealth(health))
	{
		return false;
	}
	FLRSaveSlotMetadata* metadata = SaveCatalog->FindSlot(slotId);
	if (!metadata || metadata->Health == health)
	{
		return false;
	}
	metadata->Health = health;
	FString error;
	return FLRSaveCatalogStore::CommitCatalog(*SaveCatalog, error);
}

void ULRSaveSubsystem::StartV2Load()
{
	V2OperationState = ELRSaveOperationState::ReadingPayload;
	TArray<FLRSaveSlotMetadata> candidates;
	if (ActiveV2Operation.Type == ELRSaveOperationType::Continue)
	{
		candidates = SaveCatalog->Slots;
		candidates.Sort([](const FLRSaveSlotMetadata& a, const FLRSaveSlotMetadata& b)
		{
			return a.SavedAtUtc == b.SavedAtUtc ? a.SaveSequence > b.SaveSequence : a.SavedAtUtc > b.SavedAtUtc;
		});
	}
	else if (const FLRSaveSlotMetadata* target = SaveCatalog->FindSlot(ActiveV2Operation.SlotId))
	{
		candidates.Add(*target);
	}
	if (candidates.IsEmpty())
	{
		CompleteV2Operation(ELRSaveResultCode::RejectedInvalidSlot, TEXT("No matching V2 save slot."));
		return;
	}

	ELRSaveResultCode lastCode = ELRSaveResultCode::ReadFailed;
	FString lastError;
	for (const FLRSaveSlotMetadata& metadata : candidates)
	{
		ELRSaveSlotHealth health = ELRSaveSlotHealth::Healthy;
		LoadedV2Payload = FLRSaveCatalogStore::LoadAndValidatePayload(this, metadata, health, lastCode, lastError);
		if (LoadedV2Payload)
		{
			ActiveV2Operation.SlotId = metadata.SlotId;
			V2OperationState = ELRSaveOperationState::AwaitingWorld;
			OnSaveLoadRequested.Broadcast(ActiveV2Operation.OperationId, LoadedV2Payload->Data.Player.CurrentMapId);
			return;
		}
		PersistDeterministicHealth(metadata.SlotId, health);
	}
	CompleteV2Operation(lastCode, lastError);
}

void ULRSaveSubsystem::NotifyLoadWorldReady(const FGuid operationId)
{
	if (V2OperationState != ELRSaveOperationState::AwaitingWorld
		|| ActiveV2Operation.OperationId != operationId || !LoadedV2Payload)
	{
		return;
	}
	UGameInstance* gameInstance = GetGameInstance();
	if (!gameInstance)
	{
		CompleteV2Operation(ELRSaveResultCode::ProviderUnavailable, TEXT("GameInstance is unavailable."));
		return;
	}
	V2OperationState = ELRSaveOperationState::Restoring;
	FString error;
	if (!LRSaveProviders::RestoreNonPlayer(SaveProviders, *gameInstance, LoadedV2Payload->Data, error)
		|| !LRSaveProviders::RestorePlayer(SaveProviders, *gameInstance, LoadedV2Payload->Data, error))
	{
		CompleteV2Operation(ELRSaveResultCode::ProviderUnavailable, error);
		return;
	}
	CompleteV2Operation(ELRSaveResultCode::Succeeded);
}

void ULRSaveSubsystem::NotifyLoadPreparationFailed(const FGuid operationId, const FString& diagnostic)
{
	if (V2OperationState == ELRSaveOperationState::AwaitingWorld
		&& ActiveV2Operation.OperationId == operationId)
	{
		CompleteV2Operation(ELRSaveResultCode::RejectedNotEligible, diagnostic);
	}
}

void ULRSaveSubsystem::StartV2NewGame()
{
	const ULRGameInstanceSubsystem* data = GetGameInstance()
		? GetGameInstance()->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	const ULRGameContentSet* content = data ? data->GetContentSet() : nullptr;
	if (!content || content->NewGameMapId.IsNone())
	{
		CompleteV2Operation(ELRSaveResultCode::RejectedNotEligible, TEXT("New Game map is not configured."));
		return;
	}
	PendingNewGameOperationId = ActiveV2Operation.OperationId;
	V2OperationState = ELRSaveOperationState::AwaitingWorld;
	OnSaveNewGameRequested.Broadcast(ActiveV2Operation.OperationId, content->NewGameMapId);
}

void ULRSaveSubsystem::NotifyNewGameWorldReady(const FGuid operationId)
{
	if (V2OperationState != ELRSaveOperationState::AwaitingWorld
		|| PendingNewGameOperationId != operationId || !GetGameInstance())
	{
		return;
	}
	const ULRGameInstanceSubsystem* data = GetGameInstance()->GetSubsystem<ULRGameInstanceSubsystem>();
	const ULRGameContentSet* content = data ? data->GetContentSet() : nullptr;
	const FLRMapRegistration* map = content ? content->FindMapRegistration(content->NewGameMapId) : nullptr;
	ALRCharacter* character = Cast<ALRCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	ALRSaveAnchor* anchor = map && character
		? ALRSaveAnchor::FindById(character->GetWorld(), map->DefaultStartAnchorId) : nullptr;
	if (!anchor || !character->SetActorLocationAndRotation(anchor->GetActorLocation(), anchor->GetActorRotation(),
		false, nullptr, ETeleportType::TeleportPhysics))
	{
		PendingNewGameOperationId.Invalidate();
		CompleteV2Operation(ELRSaveResultCode::ProviderUnavailable,
			TEXT("New Game default start anchor is unavailable."));
		return;
	}
	FString error;
	if (!LRSaveProviders::ResetForNewGame(SaveProviders, *GetGameInstance(), error))
	{
		PendingNewGameOperationId.Invalidate();
		CompleteV2Operation(ELRSaveResultCode::ProviderUnavailable, error);
		return;
	}
	PendingNewGameOperationId.Invalidate();
	StartV2Write();
}
