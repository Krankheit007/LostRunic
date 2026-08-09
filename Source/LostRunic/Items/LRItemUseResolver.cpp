/**
 * @file LRItemUseResolver.cpp
 * @brief 执行物品使用事务的目标匹配、标签/状态检查、预消耗、目标执行和失败回滚，保证钥匙门两种入口结算一致。
 *
 * 关联文件：LRItemUseResolver.h；所属领域：Items。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
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

/**
 * @brief 初始化子系统拥有的长期状态与事件绑定。
 * @param inventory 参与本次操作的运行时对象 `inventory`；函数会检查空值和所需接口。
 */
void ULRItemUseResolver::Initialize(ULRInventoryComponent* inventory)
{
	Inventory = inventory;
}

/**
 * @brief 执行 Resolve At Time 的纯规则或事务判定，失败时提供结构化原因。
 * @param request 不可变领域请求，包含本次操作所需的稳定 ID、来源、目标或原因。
 * @param currentTimeSeconds 时间值 `currentTimeSeconds`，单位为秒。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
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

/**
 * @brief 按稳定 ID 或运行时条件查找 Target Object，未找到时返回明确失败值。
 * @param target 本次规则检查或操作的目标对象。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
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

/**
 * @brief 创建带原因 Gameplay Tag 的结构化失败结果，并保留事务不变量。
 * @param request 不可变领域请求，包含本次操作所需的稳定 ID、来源、目标或原因。
 * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FLRItemUseResult ULRItemUseResolver::Reject(const FLRItemUseRequest& request, const FGameplayTag reason) const
{
	FLRItemUseResult result;
	result.ItemId = request.ItemId;
	result.FailureReason = reason;
	UE_LOG(LogLostRunicInteraction, Verbose, TEXT("Item=%s target=%s rejected reason=%s"),
		*request.ItemId.ToString(), *GetNameSafe(request.Target), *reason.ToString());
	return result;
}
