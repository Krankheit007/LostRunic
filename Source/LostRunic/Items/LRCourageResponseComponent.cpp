/**
 * @file LRCourageResponseComponent.cpp
 * @brief 攻击目标响应：实现 ILRAttackTarget，按调优参数执行非致死击退；免疫、状态、冷却和消费由统一物品事务管理。
 *
 * 关联文件：LRCourageResponseComponent.h；所属领域：Items。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Items/LRCourageResponseComponent.h"

#include "Core/LRGameplayTags.h"
#include "Data/LRItemDefinition.h"
#include "Data/LRGameTuningSet.h"
#include "Data/LRStateTuning.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"

/**
 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
 */
ULRCourageResponseComponent::ULRCourageResponseComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

/**
 * @brief 查询 Attack Target Tags_Implementation；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FGameplayTagContainer ULRCourageResponseComponent::GetAttackTargetTags_Implementation()
{
	FGameplayTagContainer tags;
	tags.AddTag(bImmune ? LRGameplayTags::TargetGuardCourageImmune : LRGameplayTags::TargetGuardCourageVulnerable);
	return tags;
}

/**
 * @brief 把 Apply Attack_Implementation 数据应用到运行时对象，并显式处理缺失依赖。
 * @param request 不可变领域请求，包含本次操作所需的稳定 ID、目标或原因。
 * @param definition 数据或调优来源 `definition`；调用期间只读，并按稳定 ID 解析内容。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FLRItemUseResult ULRCourageResponseComponent::ApplyAttack_Implementation(const FLRItemUseRequest& request,
	ULRItemDefinition* definition)
{
	FLRItemUseResult result;
	result.ItemId = request.ItemId;
	if (bImmune)
	{
		result.FailureReason = LRGameplayTags::ItemUseRejectImmune;
		return result;
	}

	ACharacter* character = Cast<ACharacter>(GetOwner());
	if (!character || !request.Instigator)
	{
		result.FailureReason = LRGameplayTags::ItemUseRejectExecution;
		return result;
	}
	const UGameInstance* gameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const ULRGameInstanceSubsystem* subsystem = gameInstance ? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	const ULRStateTuning* tuning = subsystem && subsystem->GetTuningSet()
		? subsystem->GetTuningSet()->State : GetDefault<ULRStateTuning>();
	const FVector direction = (character->GetActorLocation() - request.Instigator->GetActorLocation()).GetSafeNormal2D();
	TSharedPtr<FRootMotionSource_ConstantForce> knockback = MakeShared<FRootMotionSource_ConstantForce>();
	knockback->InstanceName = TEXT("LRCourageKnockback");
	knockback->Duration = tuning->CourageKnockbackDurationSeconds;
	knockback->Force = direction * tuning->CourageKnockbackSpeed;
	knockback->AccumulateMode = ERootMotionAccumulateMode::Override;
	character->GetCharacterMovement()->ApplyRootMotionSource(knockback);
	OnKnockbackApplied.Broadcast(direction);
	result.bSuccess = true;
	return result;
}
