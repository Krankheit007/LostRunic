/**
 * @file LRScreenWidget.cpp
 * @brief 实现 HUD、状态遮罩、对话/阅读、背包/笔记/收藏、暂停、存档槽和过场的控制器边界。UI 订阅领域事件并负责表现，不参与核心规则判定。
 *
 * 关联文件：LRScreenWidget.h；所属领域：UI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "UI/LRScreenWidget.h"

#include "UI/LRHUDWidgetController.h"
#include "Blueprint/WidgetTree.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"

namespace
{
	/**
	 * @brief 把二维输入方向转换为单一 EUINavigation；二维输入只取绝对值较大的轴。
	 * @param direction 本次输入、状态更新或测试使用的值；二维输入只取绝对值较大的轴。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	EUINavigation DirectionToNavigation(const FVector2D& direction)
	{
		if (FMath::IsNearlyZero(direction.SizeSquared()))
		{
			return EUINavigation::Invalid;
		}
		if (FMath::Abs(direction.X) >= FMath::Abs(direction.Y))
		{
			return direction.X > 0.f ? EUINavigation::Right : EUINavigation::Left;
		}
		return direction.Y > 0.f ? EUINavigation::Up : EUINavigation::Down;
	}
}

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

void ULRScreenWidget::SetHUDWidgetController(ULRHUDWidgetController* controller)
{
	HUDWidgetController = controller;
	OnHUDWidgetControllerReady(HUDWidgetController);
}

/**
 * @brief 把当前叙事页面数据推送到 Widget 表现，不执行剧情条件或存档规则。
 * @param presentation 本次领域操作的结构化数据 `presentation`；字段语义由对应 USTRUCT 定义。
 */
void ULRScreenWidget::PresentNarrative(const FLRNarrativePresentation& presentation)
{
	OnNarrativePresentationChanged(presentation);
}

/**
 * @brief 处理通用 UI 命令（Confirm/Cancel/PreviousTab/NextTab/PrimaryAction）；基类默认不处理任何命令。
 * @param command 本次操作使用的 `command` 枚举或模式值。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRScreenWidget::HandleUICommand_Implementation(const ELRUICommand command)
{
	return false;
}

/**
 * @brief 按方向处理导航：把方向转换为单一 EUINavigation 后交给 Slate（Widget Blueprint Navigation 元数据），
 *        Slate 成功移动焦点才返回成功；焦点无效时才执行故障恢复（RestoreFocus -> SetInitialFocus -> 自身焦点）。
 * @param direction 本次输入、状态更新或测试使用的值；二维输入只取绝对值较大的轴。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRScreenWidget::HandleNavigate(const FVector2D& direction)
{
	if (!IsScreenVisible())
	{
		return false;
	}
	// 零方向或不可判别的方向不是导航请求，不进入焦点路径（也不触发故障恢复）。
	const EUINavigation navigation = DirectionToNavigation(direction);
	if (navigation == EUINavigation::Invalid)
	{
		return false;
	}

	const int32 userIndex = GetSlateUserIndex();
	const TSharedPtr<SWidget> currentFocus = FSlateApplication::Get().GetUserFocusedWidget(userIndex);

	if (IsCurrentFocusValid())
	{
		// Slate（UMG Navigation 元数据）是唯一的方向导航系统；Slate 未移动焦点时保留当前焦点，不寻找替代方向目标。
		const TArray<EUINavigation> requests = { navigation };
		const EUINavigation navigated = FSlateApplication::Get().NavigateFromWidget(userIndex, currentFocus, requests);
		return navigated != EUINavigation::Invalid;
	}

	// 当前焦点已失效、为空或落在本 Screen 之外：故障恢复，只保证“至少存在一个合法焦点”。
	if (RestoreFocus())
	{
		return true;
	}
	if (SetInitialFocus())
	{
		return true;
	}
	FSlateApplication::Get().SetUserFocus(userIndex, TakeWidget());
	return true;
}

/**
 * @brief 设置初始焦点：恢复当前 Tab 保存的有效索引，无效时落到第一个可用条目；基类默认聚焦 Screen 自身。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRScreenWidget::SetInitialFocus()
{
	const int32 userIndex = GetSlateUserIndex();
	FSlateApplication::Get().SetUserFocus(userIndex, TakeWidget());
	return true;
}

/**
 * @brief 恢复会话内保存的焦点索引；基类默认等同 SetInitialFocus。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRScreenWidget::RestoreFocus()
{
	return SetInitialFocus();
}

/**
 * @brief 把焦点移动到指定 Widget；目标必须可见、启用且支持键盘焦点。
 * @param widget 参与本次操作的运行时对象 `widget`；函数会检查空值和所需接口。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRScreenWidget::SetFocusToWidget(UWidget* widget) const
{
	if (!widget || !widget->IsVisible() || !widget->GetIsEnabled() || !widget->SupportsKeyboardFocus())
	{
		return false;
	}
	widget->SetUserFocus(GetOwningPlayer());
	return true;
}

/**
 * @brief 查询当前 Slate User Index；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
int32 ULRScreenWidget::GetSlateUserIndex() const
{
	const ULocalPlayer* localPlayer = GetOwningLocalPlayer();
	if (!localPlayer)
	{
		return 0;
	}
	const TSharedPtr<const FSlateUser> slateUser = localPlayer->GetSlateUser();
	return slateUser.IsValid() ? slateUser->GetUserIndex() : 0;
}

/**
 * @brief 判断当前 User Focus 是否仍属于本 Screen 且可见、启用、可聚焦；Screen 自身焦点也视为有效。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRScreenWidget::IsCurrentFocusValid()
{
	if (!WidgetTree)
	{
		return false;
	}
	const TSharedPtr<SWidget> currentFocus = FSlateApplication::Get().GetUserFocusedWidget(GetSlateUserIndex());
	if (!currentFocus.IsValid())
	{
		return false;
	}
	if (currentFocus == TakeWidget())
	{
		return true;
	}
	bool bFound = false;
	WidgetTree->ForEachWidget([&bFound, &currentFocus](UWidget* widget)
	{
		if (bFound)
		{
			return;
		}
		const TSharedPtr<SWidget> cached = widget->GetCachedWidget();
		if (cached.IsValid() && cached == currentFocus)
		{
			bFound = widget->IsVisible() && widget->GetIsEnabled() && widget->SupportsKeyboardFocus();
		}
	});
	return bFound;
}
