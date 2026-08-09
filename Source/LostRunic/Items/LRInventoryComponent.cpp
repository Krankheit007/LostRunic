#include "Items/LRInventoryComponent.h"

#include "Core/LRGameplayTags.h"
#include "Data/LRGameContentSet.h"
#include "Data/LRItemDefinition.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "Items/LRItemUseResolver.h"

namespace
{
	constexpr int32 QuickSlotCount = 4;
}

ULRInventoryComponent::ULRInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	QuickSlots.SetNum(QuickSlotCount);
}

void ULRInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	const UGameInstance* gameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const ULRGameInstanceSubsystem* subsystem = gameInstance ? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	const ULRGameContentSet* contentSet = subsystem ? subsystem->GetContentSet() : nullptr;
	TArray<ULRItemDefinition*> definitions;
	if (contentSet)
	{
		for (ULRItemDefinition* definition : contentSet->Items)
		{
			definitions.Add(definition);
		}
	}
	InitializeDefinitions(definitions);
	Resolver = NewObject<ULRItemUseResolver>(this);
	Resolver->Initialize(this);
}

void ULRInventoryComponent::InitializeDefinitions(const TArray<ULRItemDefinition*>& definitions)
{
	Definitions.Reset();
	for (ULRItemDefinition* definition : definitions)
	{
		if (definition && !definition->ItemId.IsNone())
		{
			Definitions.Add(definition->ItemId, definition);
		}
	}
	if (!Resolver)
	{
		Resolver = NewObject<ULRItemUseResolver>(this);
		Resolver->Initialize(this);
	}
}

bool ULRInventoryComponent::AddItem(const FName itemId, const int32 count)
{
	if (itemId.IsNone() || count <= 0 || !Definitions.Contains(itemId))
	{
		return false;
	}
	int32& currentCount = ItemCounts.FindOrAdd(itemId);
	currentCount += count;
	OnInventoryChanged.Broadcast(itemId, currentCount);
	return true;
}

int32 ULRInventoryComponent::GetItemCount(const FName itemId) const
{
	const int32* count = ItemCounts.Find(itemId);
	return count ? *count : 0;
}

bool ULRInventoryComponent::HasItem(const FName itemId, const int32 count) const
{
	return count > 0 && GetItemCount(itemId) >= count;
}

bool ULRInventoryComponent::AssignQuickSlot(const int32 slotIndex, const FName itemId)
{
	if (!QuickSlots.IsValidIndex(slotIndex) || (!itemId.IsNone() && !HasItem(itemId)))
	{
		return false;
	}
	QuickSlots[slotIndex] = itemId;
	OnQuickSlotChanged.Broadcast(slotIndex, itemId);
	return true;
}

void ULRInventoryComponent::SelectQuickSlot(const int32 slotIndex)
{
	if (QuickSlots.IsValidIndex(slotIndex))
	{
		SelectedQuickSlot = slotIndex;
	}
}

void ULRInventoryComponent::SelectAdjacentQuickSlot(const int32 direction)
{
	SelectedQuickSlot = (SelectedQuickSlot + FMath::Sign(direction) + QuickSlotCount) % QuickSlotCount;
}

FName ULRInventoryComponent::GetQuickSlotItem(const int32 slotIndex) const
{
	return QuickSlots.IsValidIndex(slotIndex) ? QuickSlots[slotIndex] : NAME_None;
}

FLRItemUseResult ULRInventoryComponent::UseQuickSlot(const int32 slotIndex, AActor* target,
	const ELRPerceptionMode currentMode)
{
	const FName itemId = GetQuickSlotItem(slotIndex);
	return UseItem(BuildUseRequest(itemId, slotIndex, target, currentMode, ELRItemUseEntryPoint::QuickSlot));
}

FLRItemUseResult ULRInventoryComponent::UseItemFromSelector(const FName itemId, AActor* target,
	const ELRPerceptionMode currentMode)
{
	return UseItem(BuildUseRequest(itemId, INDEX_NONE, target, currentMode, ELRItemUseEntryPoint::InteractionSelector));
}

FLRItemUseRequest ULRInventoryComponent::BuildUseRequest(const FName itemId, const int32 sourceSlot, UObject* target,
	const ELRPerceptionMode currentMode, const ELRItemUseEntryPoint entryPoint) const
{
	FLRItemUseRequest request;
	request.ItemId = itemId;
	request.SourceSlot = sourceSlot;
	request.Target = target;
	request.Instigator = GetOwner();
	request.EntryPoint = entryPoint;
	request.CurrentMode = currentMode;
	request.ActionTag = LRGameplayTags::InteractionActionUse;
	return request;
}

FLRItemUseResult ULRInventoryComponent::ResolveUseRequestAtTime(const FLRItemUseRequest& request,
	const double currentTimeSeconds)
{
	return Resolver ? Resolver->ResolveAtTime(request, currentTimeSeconds) : FLRItemUseResult();
}

bool ULRInventoryComponent::AddNoteId(const FName noteId)
{
	return !noteId.IsNone() && NoteIds.Add(noteId).IsValidId();
}

bool ULRInventoryComponent::AddCollectibleId(const FName collectibleId)
{
	return !collectibleId.IsNone() && CollectibleIds.Add(collectibleId).IsValidId();
}

FGameplayTagContainer ULRInventoryComponent::GetOwnedItemTags() const
{
	FGameplayTagContainer tags;
	for (const TPair<FName, int32>& item : ItemCounts)
	{
		if (item.Value > 0)
		{
			const ULRItemDefinition* definition = FindDefinition(item.Key);
			if (definition)
			{
				tags.AppendTags(definition->ItemTags);
			}
		}
	}
	return tags;
}

ULRItemDefinition* ULRInventoryComponent::FindDefinition(const FName itemId) const
{
	const TObjectPtr<ULRItemDefinition>* definition = Definitions.Find(itemId);
	return definition ? definition->Get() : nullptr;
}

FLRItemUseResult ULRInventoryComponent::UseItem(const FLRItemUseRequest& request)
{
	const double currentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	FLRItemUseResult result = ResolveUseRequestAtTime(request, currentTime);
	OnItemUseResolved.Broadcast(result);
	return result;
}

bool ULRInventoryComponent::TryConsumeItem(const FName itemId)
{
	int32* count = ItemCounts.Find(itemId);
	if (!count || *count <= 0)
	{
		return false;
	}
	--(*count);
	OnInventoryChanged.Broadcast(itemId, *count);
	return true;
}

void ULRInventoryComponent::RestoreItem(const FName itemId)
{
	int32& count = ItemCounts.FindOrAdd(itemId);
	++count;
	OnInventoryChanged.Broadcast(itemId, count);
}
