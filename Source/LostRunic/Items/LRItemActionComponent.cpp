/**
 * @file LRItemActionComponent.cpp
 * @brief 角色上的物品玩法动作唯一入口：RequestUseItem 走交互目标，RequestAttack 经独立攻击目标解析器取目标并组装武器/空手请求；规则、执行与消费由 ULRItemUseResolver 统一管理。
 *
 * 关联文件：LRItemActionComponent.h；所属领域：Items。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Items/LRItemActionComponent.h"

#include "Core/LRGameplayTags.h"
#include "Engine/World.h"
#include "Framework/LRCharacter.h"
#include "Items/LRAttackTargetResolver.h"
#include "Items/LRInventoryComponent.h"
#include "Items/LRItemUseResolver.h"
#include "State/LRStateComponent.h"

/**
 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
 */
ULRItemActionComponent::ULRItemActionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

/**
 * @brief 在进入世界后解析运行时依赖、绑定事件并启动所需计时器；构造阶段不访问 World 或玩家对象。
 */
void ULRItemActionComponent::BeginPlay()
{
	Super::BeginPlay();
	Inventory = GetOwner() ? GetOwner()->FindComponentByClass<ULRInventoryComponent>() : nullptr;
	State = GetOwner() ? GetOwner()->FindComponentByClass<ULRStateComponent>() : nullptr;
	AttackTargetResolver = GetOwner() ? GetOwner()->FindComponentByClass<ULRAttackTargetResolver>() : nullptr;
	Resolver = NewObject<ULRItemUseResolver>(this);
	Resolver->Initialize(Inventory);
}

/**
 * @brief 注入测试依赖并建立事务；测试替身无需拥有 Actor 或 World。
 * @param inventory 参与本次操作的运行时对象 `inventory`；函数会检查空值和所需接口。
 * @param state 参与本次操作的运行时对象 `state`；函数会检查空值和所需接口。
 */
void ULRItemActionComponent::InitializeForTesting(ULRInventoryComponent* inventory, ULRStateComponent* state)
{
	Inventory = inventory;
	State = state;
	Resolver = NewObject<ULRItemUseResolver>(this);
	Resolver->Initialize(inventory);
}

/**
 * @brief 通过统一物品事务对指定交互目标使用物品；目标必须实现 ILRItemUseTarget。
 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
 * @param target 本次规则检查或操作的目标对象。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FLRItemUseResult ULRItemActionComponent::RequestUseItem(const FName itemId, AActor* target)
{
	if (!Resolver)
	{
		FLRItemUseResult result;
		result.FailureReason = LRGameplayTags::ItemUseRejectExecution;
		return result;
	}
	const double currentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	return Resolver->ResolveAtTime(
		BuildRequest(itemId, target, LRGameplayTags::InteractionActionUse, ELRItemUseEntryPoint::Interaction),
		currentTime);
}

/**
 * @brief 发起攻击：解析最近合法攻击目标，有武器时提交武器攻击，否则提交空手攻击；只有 Courage 状态且无输入阻塞时被接受。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FLRItemUseResult ULRItemActionComponent::RequestAttack()
{
	if (!Resolver || !Inventory)
	{
		FLRItemUseResult result;
		result.FailureReason = LRGameplayTags::ItemUseRejectExecution;
		return result;
	}
	if (State && !State->GetActiveBlockers().IsEmpty())
	{
		FLRItemUseResult result;
		result.FailureReason = LRGameplayTags::StateRejectBlocked;
		return result;
	}
	AActor* target = nullptr;
	if (AttackTargetResolver && !AttackTargetResolver->FindAttackTarget(GetOwner(), target))
	{
		FLRItemUseResult result;
		result.FailureReason = LRGameplayTags::ItemUseRejectTarget;
		return result;
	}
	const FName effectiveWeapon = Inventory->GetEffectiveWeapon();
	const double currentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	return Resolver->ResolveAtTime(
		BuildRequest(effectiveWeapon, target, LRGameplayTags::InteractionActionAttack, ELRItemUseEntryPoint::Attack),
		currentTime);
}

/**
 * @brief 组装统一使用请求，并从状态组件读取当前模式。
 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
 * @param target 本次规则检查或操作的目标对象。
 * @param actionTag Gameplay Tag 或标签集合，用于分类、条件、拒绝原因和可诊断事件。
 * @param entryPoint 本次操作使用的 `entryPoint` 枚举或模式值。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FLRItemUseRequest ULRItemActionComponent::BuildRequest(const FName itemId, UObject* target,
	const FGameplayTag actionTag, const ELRItemUseEntryPoint entryPoint) const
{
	FLRItemUseRequest request;
	request.ItemId = itemId;
	request.Target = target;
	request.Instigator = GetOwner();
	request.EntryPoint = entryPoint;
	request.CurrentMode = State ? State->GetCurrentMode() : ELRPerceptionMode::Normal;
	request.ActionTag = actionTag;
	return request;
}
