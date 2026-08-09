/**
 * @file LRTestItemUseTargetComponent.cpp
 * @brief 提供 LostRunic Runtime 自动化测试，覆盖调优边界、状态矩阵、交互筛选、物品双入口、守卫警戒、叙事分支和存档事务顺序。仅在 WITH_DEV_AUTOMATION_TESTS 下编译。
 *
 * 关联文件：LRTestItemUseTargetComponent.h；所属领域：Tests。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Tests/LRTestItemUseTargetComponent.h"

#include "Core/LRGameplayTags.h"

/**
 * @brief 把 Apply Item Use_Implementation 数据应用到运行时对象，并显式处理缺失依赖。
 * @param request 不可变领域请求，包含本次操作所需的稳定 ID、来源、目标或原因。
 * @param definition 数据或调优来源 `definition`；调用期间只读，并按稳定 ID 解析内容。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FLRItemUseResult ULRTestItemUseTargetComponent::ApplyItemUse_Implementation(const FLRItemUseRequest& request,
	ULRItemDefinition* definition)
{
	++ApplyCount;
	FLRItemUseResult result;
	result.ItemId = request.ItemId;
	result.bSuccess = bShouldSucceed;
	if (!result.bSuccess)
	{
		result.FailureReason = LRGameplayTags::ItemUseRejectExecution;
	}
	return result;
}
