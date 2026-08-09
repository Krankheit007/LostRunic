/**
 * @file LRTestItemUseTargetComponent.h
 * @brief 提供 LostRunic Runtime 自动化测试，覆盖调优边界、状态矩阵、交互筛选、物品双入口、守卫警戒、叙事分支和存档事务顺序。仅在 WITH_DEV_AUTOMATION_TESTS 下编译。
 *
 * 关联文件：LRTestItemUseTargetComponent.cpp；所属领域：Tests。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Components/ActorComponent.h"
#include "Items/LRItemUseTarget.h"

#include "LRTestItemUseTargetComponent.generated.h"

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS()
class ULRTestItemUseTargetComponent : public UActorComponent, public ILRItemUseTarget
{
	GENERATED_BODY()

public:
	/**
	 * @brief 查询 Item Use Target Tags_Implementation；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	virtual FGameplayTagContainer GetItemUseTargetTags_Implementation() override { return TargetTags; }
	/**
	 * @brief 测试替身记录物品目标被调用的次数，并按 bShouldSucceed 返回成功或失败结果。
	 * @param request 自动化测试构造的统一物品使用请求。
	 * @param definition 测试解析出的物品定义；用于验证生产接口签名一致。
	 * @return bShouldSucceed 为 true 时成功，否则返回执行拒绝结果。
	 */
	virtual FLRItemUseResult ApplyItemUse_Implementation(const FLRItemUseRequest& request,
		ULRItemDefinition* definition) override;

	/** Target Tags 的内部运行时数据；不参与蓝图配置。 */
	FGameplayTagContainer TargetTags;
	/** Should Succeed 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	bool bShouldSucceed = true;
	/** Apply Count 的内部运行时数据；不参与蓝图配置。 */
	int32 ApplyCount = 0;
};
