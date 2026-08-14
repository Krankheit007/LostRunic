/**
 * @file LRLocomotionComponent.h
 * @brief 根据心理状态和玩家切换请求选择潜行/走路/奔跑，以 80/150/250 cm/s 基线移动，并按移动距离和环境发布脚步噪声。玩家请求经状态步态规则验证，组件内部应用与掩体覆盖走独立通道。
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
class ULRStateComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FLRFootstepEvent, FVector, location, float, radius, FGameplayTag, noiseTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLRPaceRequestRejected, ELRMovementPace, requestedPace, FGameplayTag, reason);

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
	 * @brief 请求切换潜行与走路；受当前状态步态规则验证，Perception 强制潜行，Courage 禁止潜行，Memory 仅走路。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Movement")
	void RequestToggleSneak();

	/**
	 * @brief 请求开始奔跑；当前状态禁止奔跑时拒绝并广播拒绝原因。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Movement")
	void RequestStartRun();

	/**
	 * @brief 请求结束奔跑，恢复到奔跑前的步态。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Movement")
	void RequestStopRun();

	/**
	 * @brief 查询有效步态（掩体覆盖优先）；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Movement")
	ELRMovementPace GetPace() const { return GetEffectivePace(); }

	/**
	 * @brief 组件内部应用步态（状态同步、掩体、调试）；玩家输入请走 Request* 入口。
	 * @param newPace 本次操作使用的 `newPace` 枚举或模式值。
	 * @param source 来源 Gameplay Tag，用于日志与诊断；None 表示常规状态应用。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Movement")
	void ApplyPace(ELRMovementPace newPace, FGameplayTag source = FGameplayTag());

	/**
	 * @brief 带来源标识的临时步态覆盖（如掩体强制潜行）；清除时按当前状态重新求值合法步态。
	 * @param newPace 本次操作使用的 `newPace` 枚举或模式值。
	 * @param source 来源 Gameplay Tag，用于标识覆盖的持有者。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Movement")
	void OverridePace(ELRMovementPace newPace, FGameplayTag source);

	/**
	 * @brief 清除指定来源的临时步态覆盖；覆盖期间的基础步态可能因状态规则过期，清除后重新求值。
	 * @param source 来源 Gameplay Tag，用于标识覆盖的持有者。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Movement")
	void ClearPaceOverride(FGameplayTag source);

	/**
	 * @brief 更新 Noise Environment，并在需要时同步组件状态或广播变化事件；仅由 ALRNoiseArea 调用。
	 * @param newEnvironment 调用方提供的 `newEnvironment`，只在本次操作范围内使用。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Movement")
	void SetNoiseEnvironment(ELRNoiseEnvironment newEnvironment) { NoiseEnvironment = newEnvironment; }

	/** 当 Footstep 发生时广播；蓝图可绑定该委托以更新表现，不应在回调中改写核心规则。  */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Noise")
	FLRFootstepEvent OnFootstep;

	/** 当 Pace Request Rejected 发生时广播；蓝图可绑定该委托以更新表现，不应在回调中改写核心规则。  */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Movement")
	FLRPaceRequestRejected OnPaceRequestRejected;

private:
	/**
	 * @brief 处理 Handle State Changed 事件，将引擎回调转换为对应领域状态更新；清空掩体覆盖并按状态默认步态应用。
	 * @param currentMode 本次操作使用的 `currentMode` 枚举或模式值。
	 * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
	 */
	UFUNCTION()
	void HandleStateChanged(ELRPerceptionMode currentMode, FGameplayTag reason);

	/**
	 * @brief 查询 Effective Mode；State 组件缺失时回退 Normal（无 World 测试场景）。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	ELRPerceptionMode GetEffectiveMode() const;
	/**
	 * @brief 查询 Effective Pace；掩体等覆盖存在时返回覆盖值，否则返回基础步态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	ELRMovementPace GetEffectivePace() const;
	/**
	 * @brief 以低频计时器累计角色实际位移，达到步长后按步态×环境解析并发布脚步而不使用 Tick。
	 */
	void SampleTravelDistance();
	/**
	 * @brief 查询 Step Distance；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	float GetStepDistance() const;
	/**
	 * @brief 按有效步态同步 CharacterMovement 的 MaxWalkSpeed。
	 */
	void SyncMovementSpeed();
	/**
	 * @brief 对禁止的步态请求统一记录日志并广播拒绝事件。
	 * @param requestedPace 本次操作使用的 `requestedPace` 枚举或模式值。
	 */
	void RejectPaceRequest(ELRMovementPace requestedPace);

	/** Character 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TObjectPtr<ACharacter> Character;

	/** 运行时解析出的调优资产缓存；不序列化，不由蓝图编辑。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TObjectPtr<ULRMovementTuning> Tuning;

	/** State 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TWeakObjectPtr<ULRStateComponent> State;

	/** Pace 的内部运行时数据；不参与蓝图配置。 */
	ELRMovementPace Pace = ELRMovementPace::Walk;
	/** Pace Before Run 的内部运行时数据；不参与蓝图配置。 */
	ELRMovementPace PaceBeforeRun = ELRMovementPace::Walk;
	/** Noise Environment 的内部运行时数据；不参与蓝图配置。 */
	ELRNoiseEnvironment NoiseEnvironment = ELRNoiseEnvironment::Outdoor;
	/** Pace Override 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	ELRMovementPace PaceOverride = ELRMovementPace::Walk;
	/** Pace Override Source 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	FGameplayTag PaceOverrideSource;
	/** Last Sample Location 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	FVector LastSampleLocation = FVector::ZeroVector;
	/** Distance Since Footstep 的内部运行时数据；不参与蓝图配置。 */
	float DistanceSinceFootstep = 0.0f;
	/** Sample Timer 的运行时句柄，用于取消回调并避免 Tick；不在蓝图中配置。 */
	FTimerHandle SampleTimer;
};
