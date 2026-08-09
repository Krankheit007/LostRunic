/**
 * @file LRItemUseTarget.h
 * @brief 实现 4 格快捷栏、背包、笔记、收藏品和统一物品使用事务；快捷栏与交互后选物共用解析入口，失败时回滚消耗并返回结构化原因。
 *
 * 关联文件：Items 目录内调用该公共契约的实现文件；所属领域：Items。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Items/LRItemUseTypes.h"
#include "UObject/Interface.h"

#include "LRItemUseTarget.generated.h"

class ULRItemDefinition;

UINTERFACE(BlueprintType, meta = (DisplayName = "Lost Runic Item Use Target"))
class LOSTRUNIC_API ULRItemUseTarget : public UInterface
{
	GENERATED_BODY()
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
class LOSTRUNIC_API ILRItemUseTarget
{
	GENERATED_BODY()

public:
	/**
	 * @brief 查询 Item Use Target Tags；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Lost Runic|Item Use")
	FGameplayTagContainer GetItemUseTargetTags();

	/**
	 * @brief 把 Apply Item Use 数据应用到运行时对象，并显式处理缺失依赖。
	 * @param request 不可变领域请求，包含本次操作所需的稳定 ID、来源、目标或原因。
	 * @param definition 数据或调优来源 `definition`；调用期间只读，并按稳定 ID 解析内容。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Lost Runic|Item Use")
	FLRItemUseResult ApplyItemUse(const FLRItemUseRequest& request, ULRItemDefinition* definition);
};
