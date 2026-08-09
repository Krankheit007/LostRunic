#include "Items/LRItemUseResolver.h"

#include "Core/LRGameplayTags.h"
#include "Core/LRLog.h"
#include "Data/LRItemDefinition.h"
#include "Data/LRGameTuningSet.h"
#include "Data/LRStateTuning.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "Items/LRInventoryComponent.h"
#include "Items/LRItemUseTarget.h"

void ULRItemUseResolver::Initialize(ULRInventoryComponent* inventory)
{
	Inventory = inventory;
}

FLRItemUseResult ULRItemUseResolver::ResolveAtTime(const FLRItemUseRequest& request, const double currentTimeSeconds)
{
	ULRItemDefinition* definition = Inventory ? Inventory->FindDefinition(request.ItemId) : nullptr;
	if (!Inventory || !definition || !Inventory->HasItem(request.ItemId))
	{
		return Reject(request, LRGameplayTags::ItemUseRejectNotOwned);
	}
	if (request.EntryPoint == ELRItemUseEntryPoint::QuickSlot
		&& Inventory->GetQuickSlotItem(request.SourceSlot) != request.ItemId)
	{
		return Reject(request, LRGameplayTags::ItemUseRejectInvalidSlot);
	}
	UObject* targetObject = FindTargetObject(request.Target);
	if (!targetObject)
	{
		return Reject(request, LRGameplayTags::ItemUseRejectTarget);
	}

	const FGameplayTagContainer targetTags = ILRItemUseTarget::Execute_GetItemUseTargetTags(targetObject);
	if (!definition->AllowedActionTags.HasTag(request.ActionTag)
		|| !targetTags.HasAny(definition->AllowedTargetTags))
	{
		return Reject(request, LRGameplayTags::InteractionRejectItem);
	}

	const bool bCourageItem = definition->ItemTags.HasTag(LRGameplayTags::ItemCategoryCourageWeapon);
	const UGameInstance* gameInstance = Inventory->GetWorld() ? Inventory->GetWorld()->GetGameInstance() : nullptr;
	const ULRGameInstanceSubsystem* subsystem = gameInstance ? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	const ULRStateTuning* stateTuning = subsystem && subsystem->GetTuningSet()
		? subsystem->GetTuningSet()->State : GetDefault<ULRStateTuning>();
	if (bCourageItem && request.CurrentMode != ELRPerceptionMode::Courage)
	{
		return Reject(request, LRGameplayTags::InteractionRejectState);
	}
	if (bCourageItem && currentTimeSeconds < LastCourageUseSeconds + stateTuning->CourageAttackCooldownSeconds)
	{
		return Reject(request, LRGameplayTags::ItemUseRejectCooldown);
	}
	if (bCourageItem && targetTags.HasTag(LRGameplayTags::TargetGuardCourageImmune))
	{
		return Reject(request, LRGameplayTags::ItemUseRejectImmune);
	}

	const bool bConsumed = !definition->bConsumable || Inventory->TryConsumeItem(request.ItemId);
	if (!bConsumed)
	{
		return Reject(request, LRGameplayTags::ItemUseRejectNotOwned);
	}
	FLRItemUseResult result = ILRItemUseTarget::Execute_ApplyItemUse(targetObject, request, definition);
	result.ItemId = request.ItemId;
	result.bConsumed = result.bSuccess && definition->bConsumable;
	if (!result.bSuccess && definition->bConsumable)
	{
		Inventory->RestoreItem(request.ItemId);
	}
	if (!result.bSuccess && !result.FailureReason.IsValid())
	{
		result.FailureReason = LRGameplayTags::ItemUseRejectExecution;
	}
	else if (result.bSuccess && bCourageItem)
	{
		LastCourageUseSeconds = currentTimeSeconds;
	}
	return result;
}

UObject* ULRItemUseResolver::FindTargetObject(UObject* target) const
{
	if (!target)
	{
		return nullptr;
	}
	if (target->GetClass()->ImplementsInterface(ULRItemUseTarget::StaticClass()))
	{
		return target;
	}
	const AActor* targetActor = Cast<AActor>(target);
	if (!targetActor)
	{
		return nullptr;
	}
	for (UActorComponent* component : targetActor->GetComponents())
	{
		if (component && component->GetClass()->ImplementsInterface(ULRItemUseTarget::StaticClass()))
		{
			return component;
		}
	}
	return nullptr;
}

FLRItemUseResult ULRItemUseResolver::Reject(const FLRItemUseRequest& request, const FGameplayTag reason) const
{
	FLRItemUseResult result;
	result.ItemId = request.ItemId;
	result.FailureReason = reason;
	UE_LOG(LogLostRunicInteraction, Verbose, TEXT("Item=%s target=%s rejected reason=%s"),
		*request.ItemId.ToString(), *GetNameSafe(request.Target), *reason.ToString());
	return result;
}
