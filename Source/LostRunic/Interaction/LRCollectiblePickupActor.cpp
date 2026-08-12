/**
 * @file LRCollectiblePickupActor.cpp
 * @brief 收藏品拾取：AddCollectibleId 返回 Success 才隐藏世界 Actor；AlreadyOwned 保持 Actor 并记录明确诊断。
 *
 * 关联文件：LRCollectiblePickupActor.h；所属领域：Interaction。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Interaction/LRCollectiblePickupActor.h"

#include "Core/LRGameplayTags.h"
#include "Core/LRLog.h"
#include "Data/LRCollectibleDefinition.h"
#include "Framework/LRCharacter.h"
#include "Items/LRInventoryComponent.h"

/** Sets the content-default pickup action used by collectible Blueprints. */
ALRCollectiblePickupActor::ALRCollectiblePickupActor()
{
	FLRInteractionOption option;
	option.ActionTag = LRGameplayTags::InteractionActionPickup;
	InteractionOptions = { option };
}

/**
 * @brief 只有 AddCollectibleId 返回 Success 才隐藏 Actor 并标记一次性完成；AlreadyOwned 保持 Actor 可见。
 * @param interactor 参与本次操作的运行时对象 `interactor`；函数会检查空值和所需接口。
 * @param actionTag Gameplay Tag 或标签集合，用于分类、条件、拒绝原因和可诊断事件。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FLRInteractionResult ALRCollectiblePickupActor::ExecuteInteractionInternal(AActor* interactor,
	const FGameplayTag actionTag)
{
	FLRInteractionResult result;
	result.ActionTag = actionTag;
	const ALRCharacter* character = Cast<ALRCharacter>(interactor);
	ULRInventoryComponent* inventory = character ? character->GetInventoryComponent() : nullptr;
	if (!inventory || !CollectibleDefinition)
	{
		UE_LOG(LogLostRunicInteraction, Warning, TEXT("Collectible=%s has no inventory or collectible definition."),
			*GetNameSafe(this));
		result.FailureReason = LRGameplayTags::ItemUseRejectInvalidDefinition;
		return result;
	}

	const FName collectibleId = CollectibleDefinition->CollectibleId;
	const ELRAddCollectibleResult addResult = inventory->AddCollectibleId(collectibleId);
	if (addResult == ELRAddCollectibleResult::AlreadyOwned)
	{
		UE_LOG(LogLostRunicInteraction, Warning, TEXT("Collectible=%s id=%s already owned; actor stays visible."),
			*GetNameSafe(this), *collectibleId.ToString());
		result.FailureReason = LRGameplayTags::CollectibleRejectAlreadyOwned;
		return result;
	}
	if (addResult != ELRAddCollectibleResult::Success)
	{
		UE_LOG(LogLostRunicInteraction, Warning, TEXT("Collectible=%s id=%s rejected add result=%d."),
			*GetNameSafe(this), *collectibleId.ToString(), static_cast<int32>(addResult));
		result.FailureReason = LRGameplayTags::ItemUseRejectInvalidDefinition;
		return result;
	}

	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);
	CompleteInteraction();
	OnCollectiblePickedUp();
	result.bSuccess = true;
	return result;
}
