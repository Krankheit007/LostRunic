/**
 * @file LRNoiseEmitterComponent.h
 * @brief 实现固定/可移动躲藏点、守卫可见性接口和统一噪声发布，使守卫通过事件感知玩家而非轮询角色速度或修改基础视野。
 *
 * 关联文件：LRNoiseEmitterComponent.cpp；所属领域：Stealth。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"

#include "LRNoiseEmitterComponent.generated.h"

class ULRInteractionComponent;
class ULRLocomotionComponent;
class ULRMovementTuning;
struct FLRInteractionResult;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FLRNoiseEmitted, FVector, location, float, radius, FGameplayTag, reason);

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(ClassGroup = "Lost Runic", BlueprintType, meta = (BlueprintSpawnableComponent, DisplayName = "Lost Runic Noise Emitter"))
class LOSTRUNIC_API ULRNoiseEmitterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 */
	ULRNoiseEmitterComponent();

	/**
	 * @brief 在进入世界后解析运行时依赖、绑定事件并启动所需计时器；构造阶段不访问 World 或玩家对象。
	 */
	virtual void BeginPlay() override;
	/**
	 * @brief 解除委托并清理计时器或缓存，避免关卡切换和对象销毁后继续收到回调。
	 * @param endPlayReason Unreal 提供的结束原因，用于区分销毁、关卡切换和退出。
	 */
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

	/**
	 * @brief 发布带半径、位置和原因标签的统一噪声事件，供守卫 Hearing 感知消费。
	 * @param location 世界空间位置，Unreal 单位为厘米。
	 * @param radius 空间值 `radius`；距离和位置使用 Unreal 厘米单位。
	 * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Noise")
	void EmitNoise(FVector location, float radius, FGameplayTag reason);

	/** 当 Noise Emitted 发生时广播；蓝图可绑定该委托以更新表现，不应在回调中改写核心规则。  */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Noise")
	FLRNoiseEmitted OnNoiseEmitted;

private:
	/**
	 * @brief 处理 Handle Footstep 事件，将引擎回调转换为对应领域状态更新。
	 * @param location 世界空间位置，Unreal 单位为厘米。
	 * @param radius 空间值 `radius`；距离和位置使用 Unreal 厘米单位。
	 * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
	 */
	UFUNCTION()
	void HandleFootstep(FVector location, float radius, FGameplayTag reason);

	/**
	 * @brief 处理 Handle Interaction 事件，将引擎回调转换为对应领域状态更新。
	 * @param result 本次领域操作的结构化数据 `result`；字段语义由对应 USTRUCT 定义。
	 */
	UFUNCTION()
	void HandleInteraction(FLRInteractionResult result);

	/** Locomotion 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TObjectPtr<ULRLocomotionComponent> Locomotion;

	/** Interaction Enhanced Input Action 资产；C++ 绑定其语义，具体键位在 Mapping Context 中配置。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TObjectPtr<ULRInteractionComponent> Interaction;

	/** 运行时解析出的调优资产缓存；不序列化，不由蓝图编辑。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TObjectPtr<ULRMovementTuning> Tuning;
};
