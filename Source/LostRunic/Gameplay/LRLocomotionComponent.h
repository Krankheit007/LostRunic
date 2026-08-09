/**
 * @file LRLocomotionComponent.h
 * @brief 根据心理状态和玩家切换请求选择潜行/走路/奔跑，以 80/150/250 cm/s 基线移动，并按移动距离和环境发布脚步噪声。
 *
 * 关联文件：LRLocomotionComponent.cpp；所属领域：Gameplay。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Components/ActorComponent.h"
#include "Core/LRTypes.h"
#include "GameplayTagContainer.h"

#include "LRLocomotionComponent.generated.h"

class ACharacter;
class ULRMovementTuning;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FLRFootstepEvent, FVector, location, float, radius, FGameplayTag, noiseTag);

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(ClassGroup = "Lost Runic", BlueprintType, meta = (BlueprintSpawnableComponent, DisplayName = "Lost Runic Locomotion"))
class LOSTRUNIC_API ULRLocomotionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 */
	ULRLocomotionComponent();

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
	 * @brief 更新 Pace，并在需要时同步组件状态或广播变化事件。
	 * @param newPace 本次操作使用的 `newPace` 枚举或模式值。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Movement")
	void SetPace(ELRMovementPace newPace);

	/**
	 * @brief 在状态允许时切换潜行与走路；Perception 强制潜行，Courage 禁止潜行。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Movement")
	void ToggleSneak();

	/**
	 * @brief 开始 Start Run 流程，建立本次操作拥有的状态、委托或计时器。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Movement")
	void StartRun();

	/**
	 * @brief 结束或取消 Stop Run 流程，并清理本次操作拥有的临时状态。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Movement")
	void StopRun();

	/**
	 * @brief 查询 Pace；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Movement")
	ELRMovementPace GetPace() const { return Pace; }

	/**
	 * @brief 更新 Noise Environment，并在需要时同步组件状态或广播变化事件。
	 * @param newEnvironment 调用方提供的 `newEnvironment`，只在本次操作范围内使用。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Movement")
	void SetNoiseEnvironment(ELRNoiseEnvironment newEnvironment) { NoiseEnvironment = newEnvironment; }

	/** 当 Footstep 发生时广播；蓝图可绑定该委托以更新表现，不应在回调中改写核心规则。  */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Noise")
	FLRFootstepEvent OnFootstep;

private:
	/**
	 * @brief 以低频计时器累计角色实际位移，达到步长后发布脚步而不使用 Tick。
	 */
	void SampleTravelDistance();
	/**
	 * @brief 查询 Step Distance；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	float GetStepDistance() const;
	/**
	 * @brief 查询 Noise Radius；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	float GetNoiseRadius() const;

	/** Character 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TObjectPtr<ACharacter> Character;

	/** 运行时解析出的调优资产缓存；不序列化，不由蓝图编辑。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TObjectPtr<ULRMovementTuning> Tuning;

	/** Pace 的内部运行时数据；不参与蓝图配置。 */
	ELRMovementPace Pace = ELRMovementPace::Walk;
	/** Pace Before Run 的内部运行时数据；不参与蓝图配置。 */
	ELRMovementPace PaceBeforeRun = ELRMovementPace::Walk;
	/** Noise Environment 的内部运行时数据；不参与蓝图配置。 */
	ELRNoiseEnvironment NoiseEnvironment = ELRNoiseEnvironment::Indoor;
	/** Last Sample Location 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	FVector LastSampleLocation = FVector::ZeroVector;
	/** Distance Since Footstep 的内部运行时数据；不参与蓝图配置。 */
	float DistanceSinceFootstep = 0.0f;
	/** Sample Timer 的运行时句柄，用于取消回调并避免 Tick；不在蓝图中配置。 */
	FTimerHandle SampleTimer;
};
