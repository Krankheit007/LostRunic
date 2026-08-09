#pragma once

#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Items/LRItemUseTypes.h"
#include "Save/LRSaveTypes.h"

#include "LRInventoryComponent.generated.h"

class ULRGameContentSet;
class ULRItemDefinition;
class ULRItemUseResolver;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLRInventoryChanged, FName, itemId, int32, newCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLRQuickSlotChanged, int32, slotIndex, FName, itemId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRItemUseResolved, FLRItemUseResult, result);

/** Owns stable item IDs, four quick slots, notes, collectibles, and item-use entry points. */
UCLASS(ClassGroup = "Lost Runic", BlueprintType, meta = (BlueprintSpawnableComponent, DisplayName = "Lost Runic Inventory"))
class LOSTRUNIC_API ULRInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULRInventoryComponent();

	virtual void BeginPlay() override;

	void InitializeDefinitions(const TArray<ULRItemDefinition*>& definitions);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Inventory")
	bool AddItem(FName itemId, int32 count = 1);

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Inventory")
	int32 GetItemCount(FName itemId) const;

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Inventory")
	bool HasItem(FName itemId, int32 count = 1) const;

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Inventory|Quick Slots")
	bool AssignQuickSlot(int32 slotIndex, FName itemId);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Inventory|Quick Slots")
	void SelectQuickSlot(int32 slotIndex);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Inventory|Quick Slots")
	void SelectAdjacentQuickSlot(int32 direction);

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Inventory|Quick Slots")
	FName GetQuickSlotItem(int32 slotIndex) const;

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Inventory|Quick Slots")
	int32 GetSelectedQuickSlot() const { return SelectedQuickSlot; }

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Inventory|Use")
	FLRItemUseResult UseQuickSlot(int32 slotIndex, AActor* target, ELRPerceptionMode currentMode);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Inventory|Use")
	FLRItemUseResult UseItemFromSelector(FName itemId, AActor* target, ELRPerceptionMode currentMode);

	FLRItemUseRequest BuildUseRequest(FName itemId, int32 sourceSlot, UObject* target,
		ELRPerceptionMode currentMode, ELRItemUseEntryPoint entryPoint) const;
	FLRItemUseResult ResolveUseRequestAtTime(const FLRItemUseRequest& request, double currentTimeSeconds);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Inventory|Journal")
	bool AddNoteId(FName noteId);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Inventory|Journal")
	bool AddCollectibleId(FName collectibleId);

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Inventory")
	FGameplayTagContainer GetOwnedItemTags() const;

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Inventory")
	TArray<FName> GetOwnedItemIds() const;

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Inventory|Journal")
	TArray<FName> GetNoteIds() const;

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Inventory|Journal")
	TArray<FName> GetCollectibleIds() const;

	ULRItemDefinition* FindDefinition(FName itemId) const;

	void CaptureSaveState(FLRSaveInventoryChunk& outInventory) const;
	void RestoreSaveState(const FLRSaveInventoryChunk& savedInventory);

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Inventory")
	FLRInventoryChanged OnInventoryChanged;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Inventory")
	FLRQuickSlotChanged OnQuickSlotChanged;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Inventory|Use")
	FLRItemUseResolved OnItemUseResolved;

private:
	friend class ULRItemUseResolver;

	FLRItemUseResult UseItem(const FLRItemUseRequest& request);
	bool TryConsumeItem(FName itemId);
	void RestoreItem(FName itemId);

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<ULRItemDefinition>> Definitions;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
	TMap<FName, int32> ItemCounts;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
	TArray<FName> QuickSlots;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
	TSet<FName> NoteIds;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
	TSet<FName> CollectibleIds;

	UPROPERTY(Transient)
	TObjectPtr<ULRItemUseResolver> Resolver;

	int32 SelectedQuickSlot = 0;
};
