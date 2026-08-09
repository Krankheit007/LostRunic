#include "Tests/LRTestItemUseTargetComponent.h"

#include "Core/LRGameplayTags.h"

FLRItemUseResult ULRTestItemUseTargetComponent::ApplyItemUse_Implementation(const FLRItemUseRequest& request,
	ULRItemDefinition* definition)
{
	++ApplyCount;
	FLRItemUseResult result;
	result.ItemId = request.ItemId;
	result.bSuccess = bShouldSucceed;
	if (!result.bSuccess)
	{
		result.FailureReason = LRGameplayTags::ItemUseRejectExecution;
	}
	return result;
}
