/**
 * @file LRCourageResponseComponent.h
 * @brief 实现 4 格快捷栏、背包、笔记、收藏品和统一物品使用事务；快捷栏与交互后选物共用解析入口，失败时回滚消耗并返回结构化原因。
 *
 * 关联文件：LRCourageResponseComponent.cpp；所属领域：Items。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Components/ActorComponent.h"
#include "Items/LRItemUseTarget.h"

#include "LRCourageResponseComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRCourageKnockbackApplied, FVector, direction);

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(ClassGroup = "Lost Runic", BlueprintType, meta = (BlueprintSpawnableComponent, DisplayName = "Lost Runic Courage Response"))
class LOSTRUNIC_API ULRCourageResponseComponent : public UActorComponent, public ILRItemUseTarget
{
	GENERATED_BODY()

public:
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 */
	ULRCourageResponseComponent();

	/**
	 * @brief 查询 Item Use Target Tags_Implementation；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	virtual FGameplayTagContainer GetItemUseTargetTags_Implementation() override;
	/**
	 * @brief 校验 Courage 状态、目标免疫标签和冷却后执行非致死击退，并返回统一物品事务结果。
	 * @param request 物品 ID、入口、玩家状态与目标组成的统一使用请求。
	 * @param definition Courage 武器定义，提供允许的动作和目标标签。
	 * @return 成功时返回击退结果；失败时返回可供 UI 显示的结构化原因标签。
	 */
	virtual FLRItemUseResult ApplyItemUse_Implementation(const FLRItemUseRequest& request,
		ULRItemDefinition* definition) override;

	/** Immune 的开关；true 表示启用，false 表示禁用。 C++ 安全默认值为 `false`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Courage")
	bool bImmune = false;

	/** 当 Knockback Applied 发生时广播；蓝图可绑定该委托以更新表现，不应在回调中改写核心规则。  */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Courage")
	FLRCourageKnockbackApplied OnKnockbackApplied;
};
