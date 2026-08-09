/**
 * @file LRGuardVisibility.h
 * @brief 实现固定/可移动躲藏点、守卫可见性接口和统一噪声发布，使守卫通过事件感知玩家而非轮询角色速度或修改基础视野。
 *
 * 关联文件：Stealth 目录内调用该公共契约的实现文件；所属领域：Stealth。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "UObject/Interface.h"

#include "LRGuardVisibility.generated.h"

UINTERFACE(BlueprintType, meta = (DisplayName = "Lost Runic Guard Visibility"))
class LOSTRUNIC_API ULRGuardVisibility : public UInterface
{
	GENERATED_BODY()
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
class LOSTRUNIC_API ILRGuardVisibility
{
	GENERATED_BODY()

public:
	/**
	 * @brief 判断 Is Visible To Guard 对应条件；不产生玩法副作用。
	 * @param guard 参与本次操作的运行时对象 `guard`；函数会检查空值和所需接口。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Lost Runic|Stealth")
	bool IsVisibleToGuard(AActor* guard) const;
};
