/**
 * @file LRScreenWidget.cpp
 * @brief 实现 HUD、状态遮罩、对话/阅读、背包/笔记/收藏、暂停、存档槽和过场的控制器边界。UI 订阅领域事件并负责表现，不参与核心规则判定。
 *
 * 关联文件：LRScreenWidget.h；所属领域：UI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "UI/LRScreenWidget.h"

/**
 * @brief 在 UMG 原生初始化阶段建立 Widget 自身状态；领域事件由外部控制器绑定。
 */
void ULRScreenWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::Collapsed);
}

/**
 * @brief 更新 Screen Visible，并在需要时同步组件状态或广播变化事件。
 * @param bVisible 布尔开关 `bVisible`；true 表示启用或条件成立，false 表示禁用或条件不成立。
 */
void ULRScreenWidget::SetScreenVisible(const bool bVisible)
{
	bScreenVisible = bVisible;
	SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	OnScreenVisibilityChanged(bVisible);
}

/**
 * @brief 把当前叙事页面数据推送到 Widget 表现，不执行剧情条件或存档规则。
 * @param presentation 本次领域操作的结构化数据 `presentation`；字段语义由对应 USTRUCT 定义。
 */
void ULRScreenWidget::PresentNarrative(const FLRNarrativePresentation& presentation)
{
	OnNarrativePresentationChanged(presentation);
}
