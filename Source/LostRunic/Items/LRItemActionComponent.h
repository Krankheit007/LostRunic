/**
 * @file LRItemActionComponent.h
 * @brief 角色上的物品玩法动作唯一入口：RequestUseItem 走交互目标，RequestAttack 经独立攻击目标解析器取目标并组装武器/空手请求；规则、执行与消费由 ULRItemUseResolver 统一管理。
 *
 * 关联文件：LRItemActionComponent.cpp；所属领域：Items。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Components/ActorComponent.h"
#include "Items/LRItemUseTypes.h"

#include "LRItemActionComponent.generated.h"

class ULRAttackTargetResolver;
class ULRInventoryComponent;
class ULRItemUseResolver;
class ULRStateComponent;

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(ClassGroup = "Lost Runic", BlueprintType, meta = (BlueprintSpawnableComponent, DisplayName = "Lost Runic Item Action"))
class LOSTRUNIC_API ULRItemActionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 */
	ULRItemActionComponent();

	/**
	 * @brief 在进入世界后解析运行时依赖、绑定事件并启动所需计时器；构造阶段不访问 World 或玩家对象。
	 */
	virtual void BeginPlay() override;

	/**
	 * @brief 通过统一物品事务对指定交互目标使用物品；目标必须实现 ILRItemUseTarget。
	 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
	 * @param target 本次规则检查或操作的目标对象。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Items")
	FLRItemUseResult RequestUseItem(FName itemId, AActor* target);

	/**
	 * @brief 发起攻击：解析最近合法攻击目标，有武器时提交武器攻击，否则提交空手攻击；只有 Courage 状态且无输入阻塞时被接受。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Items")
	FLRItemUseResult RequestAttack();

	/**
	 * @brief 查询 Resolver；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	ULRItemUseResolver* GetResolver() const { return Resolver; }

	/**
	 * @brief 注入测试依赖并建立事务；测试替身无需拥有 Actor 或 World。
	 * @param inventory 参与本次操作的运行时对象 `inventory`；函数会检查空值和所需接口。
	 * @param state 参与本次操作的运行时对象 `state`；函数会检查空值和所需接口。
	 */
	void InitializeForTesting(ULRInventoryComponent* inventory, ULRStateComponent* state);

private:
	/**
	 * @brief 组装统一使用请求，并从状态组件读取当前模式。
	 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
	 * @param target 本次规则检查或操作的目标对象。
	 * @param actionTag Gameplay Tag 或标签集合，用于分类、条件、拒绝原因和可诊断事件。
	 * @param entryPoint 本次操作使用的 `entryPoint` 枚举或模式值。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	FLRItemUseRequest BuildRequest(FName itemId, UObject* target, FGameplayTag actionTag,
		ELRItemUseEntryPoint entryPoint) const;

	/** Inventory 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TObjectPtr<ULRInventoryComponent> Inventory;

	/** State 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TObjectPtr<ULRStateComponent> State;

	/** Attack Target Resolver 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TObjectPtr<ULRAttackTargetResolver> AttackTargetResolver;

	/** Resolver 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TObjectPtr<ULRItemUseResolver> Resolver;
};
