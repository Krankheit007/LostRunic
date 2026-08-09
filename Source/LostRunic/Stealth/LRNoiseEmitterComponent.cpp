/**
 * @file LRNoiseEmitterComponent.cpp
 * @brief 实现固定/可移动躲藏点、守卫可见性接口和统一噪声发布，使守卫通过事件感知玩家而非轮询角色速度或修改基础视野。
 *
 * 关联文件：LRNoiseEmitterComponent.h；所属领域：Stealth。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Stealth/LRNoiseEmitterComponent.h"

#include "Core/LRGameplayTags.h"
#include "Data/LRGameTuningSet.h"
#include "Data/LRMovementTuning.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "Gameplay/LRLocomotionComponent.h"
#include "Interaction/LRInteractionComponent.h"
#include "Interaction/LRInteractionTypes.h"
#include "Perception/AISense_Hearing.h"

/**
 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
 */
ULRNoiseEmitterComponent::ULRNoiseEmitterComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

/**
 * @brief 在进入世界后解析运行时依赖、绑定事件并启动所需计时器；构造阶段不访问 World 或玩家对象。
 */
void ULRNoiseEmitterComponent::BeginPlay()
{
	Super::BeginPlay();
	Locomotion = GetOwner() ? GetOwner()->FindComponentByClass<ULRLocomotionComponent>() : nullptr;
	Interaction = GetOwner() ? GetOwner()->FindComponentByClass<ULRInteractionComponent>() : nullptr;
	const UGameInstance* gameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const ULRGameInstanceSubsystem* subsystem = gameInstance ? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	Tuning = subsystem && subsystem->GetTuningSet() ? subsystem->GetTuningSet()->Movement : nullptr;
	if (!ensureMsgf(Locomotion && Interaction && Tuning, TEXT("%s requires locomotion, interaction, and Movement tuning."),
		*GetNameSafe(this)))
	{
		return;
	}
	Locomotion->OnFootstep.AddDynamic(this, &ULRNoiseEmitterComponent::HandleFootstep);
	Interaction->OnInteractionExecuted.AddDynamic(this, &ULRNoiseEmitterComponent::HandleInteraction);
}

/**
 * @brief 解除委托并清理计时器或缓存，避免关卡切换和对象销毁后继续收到回调。
 * @param endPlayReason Unreal 提供的结束原因，用于区分销毁、关卡切换和退出。
 */
void ULRNoiseEmitterComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	if (Locomotion)
	{
		Locomotion->OnFootstep.RemoveDynamic(this, &ULRNoiseEmitterComponent::HandleFootstep);
	}
	if (Interaction)
	{
		Interaction->OnInteractionExecuted.RemoveDynamic(this, &ULRNoiseEmitterComponent::HandleInteraction);
	}
	Super::EndPlay(endPlayReason);
}

/**
 * @brief 发布带半径、位置和原因标签的统一噪声事件，供守卫 Hearing 感知消费。
 * @param location 世界空间位置，Unreal 单位为厘米。
 * @param radius 空间值 `radius`；距离和位置使用 Unreal 厘米单位。
 * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
 */
void ULRNoiseEmitterComponent::EmitNoise(const FVector location, const float radius, const FGameplayTag reason)
{
	if (!GetWorld() || radius <= 0.0f || !reason.IsValid())
	{
		return;
	}
	UAISense_Hearing::ReportNoiseEvent(GetWorld(), location, 1.0f, GetOwner(), radius, reason.GetTagName());
	OnNoiseEmitted.Broadcast(location, radius, reason);
}

/**
 * @brief 处理 Handle Footstep 事件，将引擎回调转换为对应领域状态更新。
 * @param location 世界空间位置，Unreal 单位为厘米。
 * @param radius 空间值 `radius`；距离和位置使用 Unreal 厘米单位。
 * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
 */
void ULRNoiseEmitterComponent::HandleFootstep(const FVector location, const float radius, const FGameplayTag reason)
{
	EmitNoise(location, radius, reason);
}

/**
 * @brief 处理 Handle Interaction 事件，将引擎回调转换为对应领域状态更新。
 * @param result 本次领域操作的结构化数据 `result`；字段语义由对应 USTRUCT 定义。
 */
void ULRNoiseEmitterComponent::HandleInteraction(const FLRInteractionResult result)
{
	if (result.bSuccess && Tuning)
	{
		EmitNoise(GetOwner()->GetActorLocation(), Tuning->InteractionNoiseRadius, LRGameplayTags::NoiseInteraction);
	}
}
