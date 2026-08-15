#include "Save/LRSaveProvider.h"

#include "Data/LRGameContentSet.h"
#include "Framework/LRCharacter.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "Items/LRInventoryComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Narrative/LRDialogueSubsystem.h"
#include "Save/LRGameStatisticsSubsystem.h"
#include "Save/LRSaveAnchor.h"

namespace
{
	ALRCharacter* ResolveCharacter(UGameInstance& gameInstance)
	{
		return Cast<ALRCharacter>(UGameplayStatics::GetPlayerCharacter(&gameInstance, 0));
	}

	class FPlayerProvider final : public ILRSaveProvider
	{
	public:
		FName GetProviderId() const override { return TEXT("Player"); }
		bool Capture(UGameInstance& gameInstance, FLRSaveDataV2& data, FString& outError) const override
		{
			ALRCharacter* character = ResolveCharacter(gameInstance);
			const ULRGameInstanceSubsystem* content = gameInstance.GetSubsystem<ULRGameInstanceSubsystem>();
			if (!character || !content || !content->GetContentSet())
			{
				outError = TEXT("Player provider could not resolve character or content.");
				return false;
			}
			data.Player.CurrentMapId = content->GetContentSet()->FindMapIdForWorld(character->GetWorld());
			data.Player.Location = character->GetActorLocation();
			data.Player.Rotation = character->GetActorRotation();
			ALRSaveAnchor* anchor = ALRSaveAnchor::FindNearest(character->GetWorld(), data.Player.Location);
			if (!anchor)
			{
				outError = TEXT("Player provider could not resolve a stable SaveAnchor in the current world.");
				return false;
			}
			data.Player.ResumeAnchor.MapId = data.Player.CurrentMapId;
			data.Player.ResumeAnchor.AnchorId = anchor->GetAnchorId();
			data.Player.ResumeAnchor.Location = anchor->GetActorLocation();
			data.Player.ResumeAnchor.Rotation = anchor->GetActorRotation();
			return !data.Player.CurrentMapId.IsNone();
		}
		bool Restore(UGameInstance& gameInstance, const FLRSaveDataV2& data, FString& outError) const override
		{
			ALRCharacter* character = ResolveCharacter(gameInstance);
			if (!character)
			{
				outError = TEXT("Player provider could not resolve the destination character.");
				return false;
			}
			ALRSaveAnchor* anchor = ALRSaveAnchor::FindById(character->GetWorld(),
				data.Player.ResumeAnchor.AnchorId);
			if (!anchor)
			{
				outError = TEXT("Saved ResumeAnchor is missing from the destination world.");
				return false;
			}
			const FVector location = anchor->GetActorLocation();
			const FRotator rotation = anchor->GetActorRotation();
			return character->SetActorLocationAndRotation(location, rotation, false, nullptr,
				ETeleportType::TeleportPhysics);
		}
	};

	class FInventoryProvider final : public ILRSaveProvider
	{
	public:
		FName GetProviderId() const override { return TEXT("Inventory"); }
		bool Capture(UGameInstance& gameInstance, FLRSaveDataV2& data, FString& outError) const override;
		bool Restore(UGameInstance& gameInstance, const FLRSaveDataV2& data, FString& outError) const override;
		bool ResetForNewGame(UGameInstance& gameInstance, FString& outError) const override;
	};

	class FNotebookProvider final : public ILRSaveProvider
	{
	public:
		FName GetProviderId() const override { return TEXT("Notebook"); }
		bool Capture(UGameInstance& gameInstance, FLRSaveDataV2& data, FString& outError) const override;
		bool Restore(UGameInstance& gameInstance, const FLRSaveDataV2& data, FString& outError) const override;
		bool ResetForNewGame(UGameInstance& gameInstance, FString& outError) const override;
	};

	class FCollectibleProvider final : public ILRSaveProvider
	{
	public:
		FName GetProviderId() const override { return TEXT("Collectible"); }
		bool Capture(UGameInstance& gameInstance, FLRSaveDataV2& data, FString& outError) const override;
		bool Restore(UGameInstance& gameInstance, const FLRSaveDataV2& data, FString& outError) const override;
		bool ResetForNewGame(UGameInstance& gameInstance, FString& outError) const override;
	};

	class FStoryProvider final : public ILRSaveProvider
	{
	public:
		FName GetProviderId() const override { return TEXT("Story"); }
		bool Capture(UGameInstance& gameInstance, FLRSaveDataV2& data, FString& outError) const override;
		bool Restore(UGameInstance& gameInstance, const FLRSaveDataV2& data, FString& outError) const override;
		bool ResetForNewGame(UGameInstance& gameInstance, FString& outError) const override;
	};

	class FStatisticsProvider final : public ILRSaveProvider
	{
	public:
		FName GetProviderId() const override { return TEXT("Statistics"); }
		bool Capture(UGameInstance& gameInstance, FLRSaveDataV2& data, FString& outError) const override;
		bool Restore(UGameInstance& gameInstance, const FLRSaveDataV2& data, FString& outError) const override;
		bool ResetForNewGame(UGameInstance& gameInstance, FString& outError) const override;
	};

	ULRInventoryComponent* ResolveInventory(UGameInstance& gameInstance, FString& outError)
	{
		ALRCharacter* character = ResolveCharacter(gameInstance);
		ULRInventoryComponent* inventory = character ? character->GetInventoryComponent() : nullptr;
		if (!inventory) { outError = TEXT("Inventory provider could not resolve inventory."); }
		return inventory;
	}

#define LR_INVENTORY_PROVIDER_IMPL(Type, CaptureFn, RestoreFn, Field) \
	bool Type::Capture(UGameInstance& gameInstance, FLRSaveDataV2& data, FString& outError) const \
	{ ULRInventoryComponent* value = ResolveInventory(gameInstance, outError); if (!value) return false; value->CaptureFn(data.Field); return true; } \
	bool Type::Restore(UGameInstance& gameInstance, const FLRSaveDataV2& data, FString& outError) const \
	{ ULRInventoryComponent* value = ResolveInventory(gameInstance, outError); if (!value) return false; value->RestoreFn(data.Field); return true; }

	LR_INVENTORY_PROVIDER_IMPL(FInventoryProvider, CaptureInventorySaveState, RestoreInventorySaveState, Inventory)
	LR_INVENTORY_PROVIDER_IMPL(FNotebookProvider, CaptureNotebookSaveState, RestoreNotebookSaveState, Notebook)
	LR_INVENTORY_PROVIDER_IMPL(FCollectibleProvider, CaptureCollectibleSaveState, RestoreCollectibleSaveState, Collectible)
#undef LR_INVENTORY_PROVIDER_IMPL

	bool FInventoryProvider::ResetForNewGame(UGameInstance& gameInstance, FString& outError) const
	{
		ULRInventoryComponent* value = ResolveInventory(gameInstance, outError);
		if (!value) return false;
		value->RestoreInventorySaveState(FLRSaveInventoryChunkV2());
		return true;
	}

	bool FNotebookProvider::ResetForNewGame(UGameInstance& gameInstance, FString& outError) const
	{
		ULRInventoryComponent* value = ResolveInventory(gameInstance, outError);
		if (!value) return false;
		value->RestoreNotebookSaveState(FLRSaveNotebookChunk());
		return true;
	}

	bool FCollectibleProvider::ResetForNewGame(UGameInstance& gameInstance, FString& outError) const
	{
		ULRInventoryComponent* value = ResolveInventory(gameInstance, outError);
		if (!value) return false;
		value->RestoreCollectibleSaveState(FLRSaveCollectibleChunk());
		return true;
	}

	bool FStoryProvider::Capture(UGameInstance& gameInstance, FLRSaveDataV2& data, FString& outError) const
	{
		ULRDialogueSubsystem* dialogue = gameInstance.GetSubsystem<ULRDialogueSubsystem>();
		if (!dialogue) { outError = TEXT("Story provider could not resolve dialogue subsystem."); return false; }
		dialogue->CaptureStorySaveState(data.Story);
		return true;
	}

	bool FStoryProvider::Restore(UGameInstance& gameInstance, const FLRSaveDataV2& data, FString& outError) const
	{
		ULRDialogueSubsystem* dialogue = gameInstance.GetSubsystem<ULRDialogueSubsystem>();
		if (!dialogue) { outError = TEXT("Story provider could not resolve dialogue subsystem."); return false; }
		dialogue->RestoreStorySaveState(data.Story);
		return true;
	}

	bool FStoryProvider::ResetForNewGame(UGameInstance& gameInstance, FString& outError) const
	{
		ULRDialogueSubsystem* dialogue = gameInstance.GetSubsystem<ULRDialogueSubsystem>();
		if (!dialogue) { outError = TEXT("Story provider could not resolve dialogue subsystem."); return false; }
		dialogue->ResetForNewGame();
		return true;
	}

	bool FStatisticsProvider::Capture(UGameInstance& gameInstance, FLRSaveDataV2& data, FString& outError) const
	{
		ULRGameStatisticsSubsystem* statistics = gameInstance.GetSubsystem<ULRGameStatisticsSubsystem>();
		if (!statistics) { outError = TEXT("Statistics provider is unavailable."); return false; }
		statistics->Capture(data.Statistics);
		return true;
	}

	bool FStatisticsProvider::Restore(UGameInstance& gameInstance, const FLRSaveDataV2& data, FString& outError) const
	{
		ULRGameStatisticsSubsystem* statistics = gameInstance.GetSubsystem<ULRGameStatisticsSubsystem>();
		if (!statistics) { outError = TEXT("Statistics provider is unavailable."); return false; }
		statistics->Restore(data.Statistics);
		return true;
	}

	bool FStatisticsProvider::ResetForNewGame(UGameInstance& gameInstance, FString& outError) const
	{
		ULRGameStatisticsSubsystem* statistics = gameInstance.GetSubsystem<ULRGameStatisticsSubsystem>();
		if (!statistics) { outError = TEXT("Statistics provider is unavailable."); return false; }
		statistics->ResetForNewGame();
		return true;
	}
}

void LRSaveProviders::CreateRequired(TArray<TUniquePtr<ILRSaveProvider>>& outProviders)
{
	outProviders.Reset();
	outProviders.Emplace(MakeUnique<FPlayerProvider>());
	outProviders.Emplace(MakeUnique<FInventoryProvider>());
	outProviders.Emplace(MakeUnique<FNotebookProvider>());
	outProviders.Emplace(MakeUnique<FCollectibleProvider>());
	outProviders.Emplace(MakeUnique<FStoryProvider>());
	outProviders.Emplace(MakeUnique<FStatisticsProvider>());
}

bool LRSaveProviders::CaptureAll(const TArray<TUniquePtr<ILRSaveProvider>>& providers,
	UGameInstance& gameInstance, FLRSaveDataV2& data, FString& outError)
{
	for (const TUniquePtr<ILRSaveProvider>& provider : providers)
	{
		if (!provider->Capture(gameInstance, data, outError) && provider->IsRequired()) return false;
	}
	return true;
}

bool LRSaveProviders::RestoreNonPlayer(const TArray<TUniquePtr<ILRSaveProvider>>& providers,
	UGameInstance& gameInstance, const FLRSaveDataV2& data, FString& outError)
{
	for (const TUniquePtr<ILRSaveProvider>& provider : providers)
	{
		if (provider->GetProviderId() != TEXT("Player")
			&& !provider->Restore(gameInstance, data, outError) && provider->IsRequired()) return false;
	}
	return true;
}

bool LRSaveProviders::RestorePlayer(const TArray<TUniquePtr<ILRSaveProvider>>& providers,
	UGameInstance& gameInstance, const FLRSaveDataV2& data, FString& outError)
{
	for (const TUniquePtr<ILRSaveProvider>& provider : providers)
	{
		if (provider->GetProviderId() == TEXT("Player")) return provider->Restore(gameInstance, data, outError);
	}
	outError = TEXT("Required Player provider was not registered.");
	return false;
}

bool LRSaveProviders::ResetForNewGame(const TArray<TUniquePtr<ILRSaveProvider>>& providers,
	UGameInstance& gameInstance, FString& outError)
{
	for (const TUniquePtr<ILRSaveProvider>& provider : providers)
	{
		if (!provider->ResetForNewGame(gameInstance, outError) && provider->IsRequired()) return false;
	}
	return true;
}
