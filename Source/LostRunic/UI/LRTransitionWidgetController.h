/**
 * @file LRTransitionWidgetController.h
 * @brief 实现 HUD、状态遮罩、对话/阅读、背包/笔记/收藏、暂停、存档槽和过场的控制器边界。UI 订阅领域事件并负责表现，不参与核心规则判定。
 *
 * 关联文件：LRTransitionWidgetController.cpp；所属领域：UI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "UObject/Object.h"

#include "LRTransitionWidgetController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRTransitionVisibilityChanged, bool, bVisible);

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Transition Widget Controller"))
class LOSTRUNIC_API ULRTransitionWidgetController : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * @brief 更新 Transition Visible，并在需要时同步组件状态或广播变化事件。
	 * @param bVisible 布尔开关 `bVisible`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 */
	void SetTransitionVisible(bool bVisible);
	/**
	 * @brief 判断 Is Transition Visible 对应条件；不产生玩法副作用。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool IsTransitionVisible() const { return bTransitionVisible; }

	/** 当 Transition Visibility Changed 发生时广播；蓝图可绑定该委托以更新表现，不应在回调中改写核心规则。  */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|UI")
	FLRTransitionVisibilityChanged OnTransitionVisibilityChanged;

private:
	/** Transition Visible 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	bool bTransitionVisible = false;
};
