/**
 * @file LRPickupInteractableActor.cpp
 * @brief 可拾取物品交互：库存 AddItem 成功后才隐藏 Actor 并标记完成；InventoryFull 时 Actor 保持可见且可交互。
 *
 * 关联文件：LRPickupInteractableActor.h；所属领域：Interaction。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Interaction/LRPickupInteractableActor.h"

#include "Core/LRGameplayTags.h"
#include "Core/LRLog.h"
#include "Data/LRItemDefinition.h"
#include "Framework/LRCharacter.h"
#include "Items/LRInventoryComponent.h"

/** Sets the content-default action used by pickup Blueprints. */
ALRPickupInteractableActor::ALRPickupInteractableActor()
{
	FLRInteractionOption option;
	option.ActionTag = LRGameplayTags::InteractionActionPickup;
	InteractionOptions = { option };
}

/**
 * @brief 只有库存 AddItem 返回 Success 才隐藏 Actor、关闭碰撞并标记一次性完成。
 * @param interactor 参与本次操作的运行时对象 `interactor`；函数会检查空值和所需接口。
 * @param actionTag Gameplay Tag 或标签集合，用于分类、条件、拒绝原因和可诊断事件。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FLRInteractionResult ALRPickupInteractableActor::ExecuteInteractionInternal(AActor* interactor,
	const FGameplayTag actionTag)
{
	FLRInteractionResult result;
	result.ActionTag = actionTag;
	const ALRCharacter* character = Cast<ALRCharacter>(interactor);
	ULRInventoryComponent* inventory = character ? character->GetInventoryComponent() : nullptr;
	if (!inventory || !ItemDefinition)
	{
		UE_LOG(LogLostRunicInteraction, Warning, TEXT("Pickup=%s has no inventory or item definition."), *GetNameSafe(this));
		result.FailureReason = LRGameplayTags::ItemUseRejectInvalidDefinition;
		return result;
	}

	const ELRAddItemResult addResult = inventory->AddItem(ItemDefinition->ItemId, PickupQuantity);
	if (addResult == ELRAddItemResult::InventoryFull)
	{
		result.FailureReason = LRGameplayTags::ItemUseRejectInventoryFull;
		return result;
	}
	if (addResult == ELRAddItemResult::InvalidQuantity)
	{
		UE_LOG(LogLostRunicInteraction, Warning, TEXT("Pickup=%s item=%s rejected quantity=%d; must be positive."),
			*GetNameSafe(this), *ItemDefinition->ItemId.ToString(), PickupQuantity);
		result.FailureReason = LRGameplayTags::ItemUseRejectInvalidQuantity;
		return result;
	}
	if (addResult != ELRAddItemResult::Success)
	{
		UE_LOG(LogLostRunicInteraction, Warning, TEXT("Pickup=%s item=%s rejected add result=%d."),
			*GetNameSafe(this), *ItemDefinition->ItemId.ToString(), static_cast<int32>(addResult));
		result.FailureReason = LRGameplayTags::ItemUseRejectInvalidDefinition;
		return result;
	}

	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);
	CompleteInteraction();
	OnPickupCompleted();
	result.bSuccess = true;
	return result;
}
