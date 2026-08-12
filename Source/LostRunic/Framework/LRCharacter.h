/**
 * @file LRCharacter.h
 * @brief 连接 LostRunic 的 Gameplay Framework：GameMode 管理单机世界规则，PlayerController 解释 Enhanced Input 与 UI 模式，Character 只组合能力组件，GameInstanceSubsystem 提供跨地图内容与调优配置。
 *
 * 关联文件：LRCharacter.cpp；所属领域：Framework。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "GameFramework/Character.h"

#include "LRCharacter.generated.h"

class UCameraComponent;
class ULRInteractionComponent;
class ULRInventoryComponent;
class ULRItemActionComponent;
class ULRHideComponent;
class ULRLocomotionComponent;
class ULRNoiseEmitterComponent;
class ULRStateComponent;
class ULRStatePresentationComponent;
class UAIPerceptionStimuliSourceComponent;
class USpringArmComponent;

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Character"))
class LOSTRUNIC_API ALRCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 */
	ALRCharacter();

	/**
	 * @brief 把二维移动语义转换为角色世界方向输入；速度限制由 LRLocomotionComponent 和调优资产维护。
	 * @param input 输入动作或数值 `input`；不包含写死的具体键位。
	 */
	void ApplyMoveInput(const FVector2D& input);

	/**
	 * @brief 查询 Locomotion Component；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Movement")
	ULRLocomotionComponent* GetLocomotionComponent() const { return Locomotion; }

	/**
	 * @brief 查询 State Component；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|State")
	ULRStateComponent* GetStateComponent() const { return State; }

	/**
	 * @brief 查询 Interaction Component；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Interaction")
	ULRInteractionComponent* GetInteractionComponent() const { return Interaction; }

	/**
	 * @brief 查询 Inventory Component；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Inventory")
	ULRInventoryComponent* GetInventoryComponent() const { return Inventory; }

	/**
	 * @brief 查询 Item Action Component；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Items")
	ULRItemActionComponent* GetItemActionComponent() const { return ItemAction; }

	/**
	 * @brief 稳定玩法动作入口：使用物品；语义合法性由 ItemActionComponent 与统一事务决定。
	 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
	 * @param target 本次规则检查或操作的目标对象。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Items")
	FLRItemUseResult RequestUseItem(FName itemId, AActor* target);

	/**
	 * @brief 稳定玩法动作入口：发起攻击；语义合法性由 ItemActionComponent 与统一事务决定。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Items")
	FLRItemUseResult RequestAttack();

	/**
	 * @brief 查询 Hide Component；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Stealth")
	ULRHideComponent* GetHideComponent() const { return Hide; }

	/** Returns the component that emits state-change presentation requests to Blueprint. */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|State|Presentation")
	ULRStatePresentationComponent* GetStatePresentationComponent() const { return StatePresentation; }

	/** Returns the component responsible for publishing gameplay noise events. */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Stealth")
	ULRNoiseEmitterComponent* GetNoiseEmitterComponent() const { return NoiseEmitter; }

	/** Returns the designer-configurable top-down camera boom. */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Camera")
	USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns the local player's top-down camera component. */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Camera")
	UCameraComponent* GetTopDownCamera() const { return Camera; }

private:
	/** Camera Boom 的领域数据，由所属类型负责维护和校验。 仅在蓝图或详情面板中查看，不可编辑。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	/** Camera 的领域数据，由所属类型负责维护和校验。 仅在蓝图或详情面板中查看，不可编辑。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> Camera;

	/** Locomotion 的领域数据，由所属类型负责维护和校验。 仅在蓝图或详情面板中查看，不可编辑。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULRLocomotionComponent> Locomotion;

	/** State 的领域数据，由所属类型负责维护和校验。 仅在蓝图或详情面板中查看，不可编辑。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULRStateComponent> State;

	/** State Presentation 的领域数据，由所属类型负责维护和校验。 仅在蓝图或详情面板中查看，不可编辑。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULRStatePresentationComponent> StatePresentation;

	/** Inventory 的领域数据，由所属类型负责维护和校验。 仅在蓝图或详情面板中查看，不可编辑。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULRInventoryComponent> Inventory;

	/** Item Action 的领域数据，由所属类型负责维护和校验。 仅在蓝图或详情面板中查看，不可编辑。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULRItemActionComponent> ItemAction;

	/** Attack Target Resolver 的领域数据，由所属类型负责维护和校验。 仅在蓝图或详情面板中查看，不可编辑。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class ULRAttackTargetResolver> AttackTargetResolver;

	/** Interaction Enhanced Input Action 资产；C++ 绑定其语义，具体键位在 Mapping Context 中配置。 仅在蓝图或详情面板中查看，不可编辑。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULRInteractionComponent> Interaction;

	/** Hide 的领域数据，由所属类型负责维护和校验。 仅在蓝图或详情面板中查看，不可编辑。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULRHideComponent> Hide;

	/** Noise Emitter 的领域数据，由所属类型负责维护和校验。 仅在蓝图或详情面板中查看，不可编辑。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULRNoiseEmitterComponent> NoiseEmitter;

	/** Stimuli Source 的领域数据，由所属类型负责维护和校验。 仅在蓝图或详情面板中查看，不可编辑。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAIPerceptionStimuliSourceComponent> StimuliSource;
};
