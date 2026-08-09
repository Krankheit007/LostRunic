/**
 * @file LRItemUseResolver.h
 * @brief 执行物品使用事务的目标匹配、标签/状态检查、预消耗、目标执行和失败回滚，保证钥匙门两种入口结算一致。
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

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Item Use Resolver"))
class LOSTRUNIC_API ULRItemUseResolver : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * @brief 初始化子系统拥有的长期状态与事件绑定。
	 * @param inventory 参与本次操作的运行时对象 `inventory`；函数会检查空值和所需接口。
	 */
	void Initialize(ULRInventoryComponent* inventory);
	/**
	 * @brief 执行 Resolve At Time 的纯规则或事务判定，失败时提供结构化原因。
	 * @param request 不可变领域请求，包含本次操作所需的稳定 ID、来源、目标或原因。
	 * @param currentTimeSeconds 时间值 `currentTimeSeconds`，单位为秒。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	FLRItemUseResult ResolveAtTime(const FLRItemUseRequest& request, double currentTimeSeconds);

private:
	/**
	 * @brief 按稳定 ID 或运行时条件查找 Target Object，未找到时返回明确失败值。
	 * @param target 本次规则检查或操作的目标对象。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UObject* FindTargetObject(UObject* target) const;
	/**
	 * @brief 创建带原因 Gameplay Tag 的结构化失败结果，并保留事务不变量。
	 * @param request 不可变领域请求，包含本次操作所需的稳定 ID、来源、目标或原因。
	 * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	FLRItemUseResult Reject(const FLRItemUseRequest& request, FGameplayTag reason) const;

	/** Inventory 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TObjectPtr<ULRInventoryComponent> Inventory;

	/** Last Courage Use Seconds 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	double LastCourageUseSeconds = -DBL_MAX;
};
