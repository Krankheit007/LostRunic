/**
 * @file LRItemUseResolver.cpp
 * @brief 统一物品事务：校验、目标检查、执行、成功消费与结构化结果；Interaction 与 Attack 各自拥有独立目标接口和筛选语义。
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
#include "Items/LRAttackTarget.h"
#include "Items/LRInventoryComponent.h"
#include "Items/LRItemUseTarget.h"

/**
 * @brief 初始化事务依赖：库存是定义、持有和消费的唯一权威来源。
 * @param inventory 参与本次操作的运行时对象 `inventory`；函数会检查空值和所需接口。
 */
void ULRItemUseResolver::Initialize(ULRInventoryComponent* inventory)
{
	Inventory = inventory;
}

/**
 * @brief 执行统一物品事务；Interaction 与 Attack 各自使用独立目标接口，只有目标成功后消费一次性物品。
 * @param request 不可变领域请求，包含本次操作所需的稳定 ID、目标或原因。
 * @param currentTimeSeconds 时间值 `currentTimeSeconds`，单位为秒。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FLRItemUseResult ULRItemUseResolver::ResolveAtTime(const FLRItemUseRequest& request,
	const double currentTimeSeconds)
{
	const bool bAttackEntry = request.EntryPoint == ELRItemUseEntryPoint::Attack;
	const bool bEmptyHanded = bAttackEntry && request.ItemId.IsNone();

	ULRItemDefinition* definition = nullptr;
	bool bConsumable = false;
	if (!bEmptyHanded)
	{
		if (!Inventory)
		{
			return Reject(request, LRGameplayTags::ItemUseRejectNotOwned);
		}
		definition = Inventory->FindDefinition(request.ItemId);
		if (!definition || !Inventory->HasItem(request.ItemId))
		{
			return Reject(request, LRGameplayTags::ItemUseRejectNotOwned);
		}
		bConsumable = definition->bConsumable;
		if (!definition->AllowedActionTags.HasTag(request.ActionTag))
		{
			return Reject(request, bAttackEntry ? LRGameplayTags::ItemUseRejectInvalidAttackItem
				: LRGameplayTags::InteractionRejectItem);
		}
		if (bAttackEntry && !definition->ItemTags.HasTag(LRGameplayTags::ItemCategoryWeapon))
		{
			return Reject(request, LRGameplayTags::ItemUseRejectInvalidAttackItem);
		}
	}

	UObject* targetObject = bAttackEntry ? FindAttackTargetObject(request.Target) : FindItemUseTargetObject(request.Target);
	if (!targetObject)
	{
		return Reject(request, LRGameplayTags::ItemUseRejectTarget);
	}
	if (!bAttackEntry && request.ActionTag == LRGameplayTags::InteractionActionUse)
	{
		const FGameplayTagContainer targetTags = ILRItemUseTarget::Execute_GetItemUseTargetTags(targetObject);
		if (!targetTags.HasAny(definition->AllowedTargetTags))
		{
			return Reject(request, LRGameplayTags::InteractionRejectItem);
		}
	}

	if (bAttackEntry)
	{
		const ULRStateTuning& stateTuning = GetEffectiveStateTuning();
		if (request.CurrentMode != ELRPerceptionMode::Courage)
		{
			return Reject(request, LRGameplayTags::ItemUseRejectAttackState);
		}
		if (currentTimeSeconds < LastAttackSeconds + stateTuning.CourageAttackCooldownSeconds)
		{
			return Reject(request, LRGameplayTags::ItemUseRejectCooldown);
		}
		const FGameplayTagContainer targetTags = ILRAttackTarget::Execute_GetAttackTargetTags(targetObject);
		if (targetTags.HasTag(LRGameplayTags::TargetGuardCourageImmune))
		{
			return Reject(request, LRGameplayTags::ItemUseRejectImmune);
		}
	}

	FLRItemUseResult result;
	if (bAttackEntry)
	{
		result = ILRAttackTarget::Execute_ApplyAttack(targetObject, request, definition);
	}
	else
	{
		result = ILRItemUseTarget::Execute_ApplyItemUse(targetObject, request, definition);
	}
	result.ItemId = request.ItemId;
	result.bConsumed = result.bSuccess && bConsumable;
	if (result.bSuccess)
	{
		if (bConsumable && !Inventory->TryConsumeItem(request.ItemId))
		{
			result.bSuccess = false;
			result.bConsumed = false;
			result.FailureReason = LRGameplayTags::ItemUseRejectNotOwned;
		}
		else if (bAttackEntry)
		{
			LastAttackSeconds = currentTimeSeconds;
		}
	}
	if (!result.bSuccess && !result.FailureReason.IsValid())
	{
		result.FailureReason = LRGameplayTags::ItemUseRejectExecution;
	}
	OnItemUseResolved.Broadcast(result);
	return result;
}

/**
 * @brief 按接口或组件查找 Interaction 目标；只接受 ILRItemUseTarget。
 * @param target 本次规则检查或操作的目标对象。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
UObject* ULRItemUseResolver::FindItemUseTargetObject(UObject* target) const
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
 * @brief 按接口或组件查找攻击目标；只接受 ILRAttackTarget。
 * @param target 本次规则检查或操作的目标对象。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
UObject* ULRItemUseResolver::FindAttackTargetObject(UObject* target) const
{
	if (!target)
	{
		return nullptr;
	}
	if (target->GetClass()->ImplementsInterface(ULRAttackTarget::StaticClass()))
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
		if (component && component->GetClass()->ImplementsInterface(ULRAttackTarget::StaticClass()))
		{
			return component;
		}
	}
	return nullptr;
}

/**
 * @brief 创建带原因 Gameplay Tag 的结构化失败结果，并保留事务不变量。
 * @param request 不可变领域请求，包含本次操作所需的稳定 ID、目标或原因。
 * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FLRItemUseResult ULRItemUseResolver::Reject(const FLRItemUseRequest& request, const FGameplayTag reason) const
{
	FLRItemUseResult result;
	result.ItemId = request.ItemId;
	result.FailureReason = reason;
	UE_LOG(LogLostRunicInteraction, Verbose, TEXT("Item=%s entry=%d target=%s rejected reason=%s"),
		*request.ItemId.ToString(), static_cast<int32>(request.EntryPoint), *GetNameSafe(request.Target),
		*reason.ToString());
	return result;
}

/**
 * @brief 查询 Effective State Tuning；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
const ULRStateTuning& ULRItemUseResolver::GetEffectiveStateTuning() const
{
	const UGameInstance* gameInstance = Inventory && Inventory->GetWorld()
		? Inventory->GetWorld()->GetGameInstance() : nullptr;
	const ULRGameInstanceSubsystem* subsystem = gameInstance ? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	const ULRStateTuning* tuning = subsystem && subsystem->GetTuningSet()
		? subsystem->GetTuningSet()->State : nullptr;
	return tuning ? *tuning : *GetDefault<ULRStateTuning>();
}
