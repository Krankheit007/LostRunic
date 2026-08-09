/**
 * @file LRHideComponent.h
 * @brief 实现固定/可移动躲藏点、守卫可见性接口和统一噪声发布，使守卫通过事件感知玩家而非轮询角色速度或修改基础视野。
 *
 * 关联文件：LRHideComponent.cpp；所属领域：Stealth。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Components/ActorComponent.h"
#include "Stealth/LRGuardVisibility.h"

#include "LRHideComponent.generated.h"

class ACharacter;
class ALRHidePoint;
class ULRStateComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLRHiddenStateChanged, bool, bHidden, ALRHidePoint*, hidePoint);

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(ClassGroup = "Lost Runic", BlueprintType, meta = (BlueprintSpawnableComponent, DisplayName = "Lost Runic Hide"))
class LOSTRUNIC_API ULRHideComponent : public UActorComponent, public ILRGuardVisibility
{
	GENERATED_BODY()

public:
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 */
	ULRHideComponent();

	/**
	 * @brief 在进入世界后解析运行时依赖、绑定事件并启动所需计时器；构造阶段不访问 World 或玩家对象。
	 */
	virtual void BeginPlay() override;
	/**
	 * @brief 判断 Is Visible To Guard_Implementation 对应条件；不产生玩法副作用。
	 * @param guard 参与本次操作的运行时对象 `guard`；函数会检查空值和所需接口。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	virtual bool IsVisibleToGuard_Implementation(AActor* guard) const override;

	/**
	 * @brief 进入指定躲藏点，应用固定或可移动掩体规则，并向守卫可见性接口报告隐藏。
	 * @param hidePoint 参与本次操作的运行时对象 `hidePoint`；函数会检查空值和所需接口。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Stealth")
	bool EnterHidePoint(ALRHidePoint* hidePoint);

	/**
	 * @brief 退出当前躲藏点，恢复移动限制和守卫可见性。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Stealth")
	bool ExitHidePoint();

	/**
	 * @brief 判断 Is Hidden 对应条件；不产生玩法副作用。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Stealth")
	bool IsHidden() const { return CurrentHidePoint.IsValid(); }

	/**
	 * @brief 查询 Current Hide Point；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Stealth")
	ALRHidePoint* GetCurrentHidePoint() const { return CurrentHidePoint.Get(); }

	/** 当 Hidden State Changed 发生时广播；蓝图可绑定该委托以更新表现，不应在回调中改写核心规则。  */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Stealth")
	FLRHiddenStateChanged OnHiddenStateChanged;

private:
	/** Character 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TObjectPtr<ACharacter> Character;

	/** State 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TObjectPtr<ULRStateComponent> State;

	/** Current Hide Point 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TWeakObjectPtr<ALRHidePoint> CurrentHidePoint;

	/** Movement Locked By Hide 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	bool bMovementLockedByHide = false;
};
