/**
 * @file LRTransitionWidgetController.cpp
 * @brief 实现 HUD、状态遮罩、对话/阅读、背包/笔记/收藏、暂停、存档槽和过场的控制器边界。UI 订阅领域事件并负责表现，不参与核心规则判定。
 *
 * 关联文件：LRTransitionWidgetController.h；所属领域：UI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "UI/LRTransitionWidgetController.h"

/**
 * @brief 更新 Transition Visible，并在需要时同步组件状态或广播变化事件。
 * @param bVisible 布尔开关 `bVisible`；true 表示启用或条件成立，false 表示禁用或条件不成立。
 */
void ULRTransitionWidgetController::SetTransitionVisible(const bool bVisible)
{
	if (bTransitionVisible == bVisible)
	{
		return;
	}
	bTransitionVisible = bVisible;
	OnTransitionVisibilityChanged.Broadcast(bTransitionVisible);
}
