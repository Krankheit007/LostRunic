/**
 * @file LRAttackTarget.h
 * @brief 定义攻击目标接口 ILRAttackTarget；与 ILRItemUseTarget 分离，门、笔记、拾取物等普通物品目标永远不参与攻击筛选。
 *
 * 关联文件：LRAttackTargetResolver.cpp、LRCourageResponseComponent；所属领域：Items。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Items/LRItemUseTypes.h"
#include "UObject/Interface.h"

#include "LRAttackTarget.generated.h"

class ULRItemDefinition;

UINTERFACE(BlueprintType, meta = (DisplayName = "Lost Runic Attack Target"))
class LOSTRUNIC_API ULRAttackTarget : public UInterface
{
	GENERATED_BODY()
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
class LOSTRUNIC_API ILRAttackTarget
{
	GENERATED_BODY()

public:
	/**
	 * @brief 查询 Attack Target Tags；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Lost Runic|Attack")
	FGameplayTagContainer GetAttackTargetTags();

	/**
	 * @brief 执行攻击效果；消耗、冷却和状态判定仍由 ULRItemUseResolver 统一管理。
	 * @param request 不可变领域请求，包含本次操作所需的稳定 ID、目标或原因。
	 * @param definition 已通过武器标签检查的物品定义；空手攻击时为 nullptr。
	 * @return 返回攻击执行结果及原因标签，供统一事务决定提交或失败。
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Lost Runic|Attack")
	FLRItemUseResult ApplyAttack(const FLRItemUseRequest& request, ULRItemDefinition* definition);
};
