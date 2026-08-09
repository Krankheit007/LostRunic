#include "Items/LRItemUseTargetActor.h"

#include "Core/LRGameplayTags.h"
#include "Data/LRItemDefinition.h"

ALRItemUseTargetActor::ALRItemUseTargetActor()
{
	PrimaryActorTick.bCanEverTick = false;
	InteractionOption.ActionTag = LRGameplayTags::InteractionActionUse;
}

TArray<FLRInteractionOption> ALRItemUseTargetActor::GetInteractionOptions_Implementation(AActor* interactor)
{
	return bCompleted && bOneShot ? TArray<FLRInteractionOption>() : TArray<FLRInteractionOption>({ InteractionOption });
}

FVector ALRItemUseTargetActor::GetInteractionLocation_Implementation()
{
	return GetActorLocation();
}

FLRInteractionResult ALRItemUseTargetActor::ExecuteInteraction_Implementation(AActor* interactor,
	const FGameplayTag actionTag)
{
	FLRInteractionResult result;
	result.ActionTag = actionTag;
	result.FailureReason = bCompleted && bOneShot ? LRGameplayTags::InteractionRejectCompleted
		: LRGameplayTags::InteractionRejectItem;
	return result;
}

FGameplayTagContainer ALRItemUseTargetActor::GetItemUseTargetTags_Implementation()
{
	return TargetTags;
}

FLRItemUseResult ALRItemUseTargetActor::ApplyItemUse_Implementation(const FLRItemUseRequest& request,
	ULRItemDefinition* definition)
{
	FLRItemUseResult result;
	result.ItemId = request.ItemId;
	if (bCompleted && bOneShot)
	{
		result.FailureReason = LRGameplayTags::InteractionRejectCompleted;
		return result;
	}
	bCompleted = true;
	result.bSuccess = true;
	result.EventId = EventId;
	OnItemUseApplied(request, definition);
	return result;
}
