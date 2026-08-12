/**
 * @file LRItemUseResolver.h
 * @brief 统一物品事务：入口与请求格式校验、定义与持有检查、Action 能力声明、状态规则、对应入口目标检查、执行、成功后消费、失败保持原库存、广播结构化结果。
 *
 * 关联文件：LRItemUseResolver.cpp；所属领域：Items。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Items/LRItemUseTypes.h"
#include "UObject/Object.h"

#include "LRItemUseResolver.generated.h"

class ULRInventoryComponent;
class ULRItemDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRItemUseResolved, FLRItemUseResult, result);

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Item Use Resolver"))
class LOSTRUNIC_API ULRItemUseResolver : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * @brief 初始化事务依赖：库存是定义、持有和消费的唯一权威来源。
	 * @param inventory 参与本次操作的运行时对象 `inventory`；函数会检查空值和所需接口。
	 */
	void Initialize(ULRInventoryComponent* inventory);

	/**
	 * @brief 执行统一物品事务；Interaction 与 Attack 各自使用独立目标接口，只有目标成功后消费一次性物品。
	 * @param request 不可变领域请求，包含本次操作所需的稳定 ID、目标或原因。
	 * @param currentTimeSeconds 时间值 `currentTimeSeconds`，单位为秒。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	FLRItemUseResult ResolveAtTime(const FLRItemUseRequest& request, double currentTimeSeconds);

	/** 当 Item Use Resolved 发生时广播；蓝图可绑定该委托以更新表现，不应在回调中改写核心规则。  */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Items")
	FLRItemUseResolved OnItemUseResolved;

private:
	/**
	 * @brief 按接口或组件查找 Interaction 目标；只接受 ILRItemUseTarget。
	 * @param target 本次规则检查或操作的目标对象。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UObject* FindItemUseTargetObject(UObject* target) const;
	/**
	 * @brief 按接口或组件查找攻击目标；只接受 ILRAttackTarget。
	 * @param target 本次规则检查或操作的目标对象。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UObject* FindAttackTargetObject(UObject* target) const;
	/**
	 * @brief 创建带原因 Gameplay Tag 的结构化失败结果，并保留事务不变量。
	 * @param request 不可变领域请求，包含本次操作所需的稳定 ID、目标或原因。
	 * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	FLRItemUseResult Reject(const FLRItemUseRequest& request, FGameplayTag reason) const;
	/**
	 * @brief 查询 Effective State Tuning；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	const class ULRStateTuning& GetEffectiveStateTuning() const;

	/** Inventory 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TObjectPtr<ULRInventoryComponent> Inventory;

	/** Last Attack Seconds 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	double LastAttackSeconds = -DBL_MAX;
};
