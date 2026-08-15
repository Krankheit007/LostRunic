/**
 * @file LRPlayerControllerUI.cpp
 * @brief 连接 LostRunic 的 Gameplay Framework：GameMode 管理单机世界规则，PlayerController 解释 Enhanced Input 与 UI 模式，Character 只组合能力组件，GameInstanceSubsystem 提供跨地图内容与调优配置。
 *
 * 关联文件：Framework 目录内调用该公共契约的实现文件；所属领域：Framework。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Framework/LRPlayerController.h"

#include "Framework/LRCharacter.h"
#include "InputActionValue.h"
#include "Interaction/LRInteractionComponent.h"
#include "Items/LRInventoryComponent.h"
#include "State/LRStateComponent.h"
#include "UI/LRHUD.h"
#include "UI/LRPlayerUIComponent.h"
#include "UI/LRScreenWidget.h"

/**
 * @brief 打开指定背包、笔记、收藏、暂停或存档页面，并切换到 Menu 输入上下文。
 * @param screen 本次操作使用的 `screen` 枚举或模式值。
 */
void ALRPlayerController::OpenMenuScreen(const ELRScreenType screen)
{
	if (PlayerUI)
	{
		PlayerUI->OpenMenuScreen(screen);
	}
}

/**
 * @brief 关闭当前菜单层并恢复 Gameplay 输入上下文，同时抑制切换时仍按住的按键。
 */
void ALRPlayerController::CloseMenuScreen()
{
	if (PlayerUI)
	{
		PlayerUI->CloseMenuScreen();
	}
}

/**
 * @brief 执行 Use Inventory Item From Menu 的玩法动作；输入层只提供语义，合法性由对应领域组件决定。
 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FLRItemUseResult ALRPlayerController::UseInventoryItemFromMenu(const FName itemId)
{
	return PlayerUI ? PlayerUI->UseInventoryItem(itemId) : FLRItemUseResult();
}

/**
 * @brief 处理 Handle Confirm 事件，将引擎回调转换为对应领域状态更新。
 */
void ALRPlayerController::HandleConfirm()
{
	if (PlayerUI)
	{
		PlayerUI->HandleConfirm();
	}
}

/**
 * @brief 处理 Handle Cancel 事件，将引擎回调转换为对应领域状态更新。
 */
void ALRPlayerController::HandleCancel()
{
	if (PlayerUI)
	{
		PlayerUI->HandleCancel();
	}
}

/**
 * @brief 处理 Handle Open Inventory 事件：仅在 Gameplay 模式打开统一菜单背包页；菜单已打开时不处理。
 */
void ALRPlayerController::HandleOpenInventory()
{
	if (PlayerUI)
	{
		PlayerUI->HandleOpenInventory();
	}
}

/**
 * @brief 处理 Handle Pause 事件，将引擎回调转换为对应领域状态更新。
 */
void ALRPlayerController::HandlePause()
{
	if (PlayerUI)
	{
		PlayerUI->HandlePause();
	}
}

/**
 * @brief 处理 Handle Navigate 事件：方向导航交给 PlayerUIComponent 路由到当前可聚焦 Screen。
 * @param value 本次输入、状态更新或测试使用的值。
 */
void ALRPlayerController::HandleNavigate(const FInputActionValue& value)
{
	if (PlayerUI)
	{
		PlayerUI->HandleNavigate(value.Get<FVector2D>());
	}
}

/**
 * @brief 处理 Handle Previous Tab 事件：切换统一菜单上一页。
 */
void ALRPlayerController::HandlePreviousTab()
{
	if (PlayerUI)
	{
		PlayerUI->HandlePreviousTab();
	}
}

/**
 * @brief 处理 Handle Next Tab 事件：切换统一菜单下一页。
 */
void ALRPlayerController::HandleNextTab()
{
	if (PlayerUI)
	{
		PlayerUI->HandleNextTab();
	}
}

/**
 * @brief 处理 Handle UI Primary Action 事件：对当前 UI 条目执行主要业务动作（本界面为装备焦点武器）。
 */
void ALRPlayerController::HandleUIPrimaryAction()
{
	if (PlayerUI)
	{
		PlayerUI->HandleUIPrimaryAction();
	}
}

/**
 * @brief 处理 Handle UI Delete 事件，将删除语义交给当前菜单 Screen。
 */
void ALRPlayerController::HandleUIDelete()
{
	if (PlayerUI)
	{
		PlayerUI->HandleUIDelete();
	}
}

/**
 * @brief 根据 Gameplay、Dialogue、Menu、Transition 模式设置鼠标、焦点和输入捕获。
 * @param newMode 本次操作使用的 `newMode` 枚举或模式值。
 */
void ALRPlayerController::ConfigureViewportInput(const ELRInputMode newMode)
{
	if (newMode == ELRInputMode::Gameplay)
	{
		bShowMouseCursor = false;
		FInputModeGameOnly inputMode;
		SetInputMode(inputMode);
		return;
	}

	ALRHUD* hud = GetLRHUD();
	ULRScreenWidget* focusWidget = hud ? hud->GetFocusableScreen(newMode) : nullptr;
	if (newMode == ELRInputMode::Transition)
	{
		FInputModeUIOnly inputMode;
		if (focusWidget)
		{
			inputMode.SetWidgetToFocus(focusWidget->TakeWidget());
		}
		SetInputMode(inputMode);
		bShowMouseCursor = false;
		return;
	}

	FInputModeGameAndUI inputMode;
	inputMode.SetHideCursorDuringCapture(newMode != ELRInputMode::Menu);
	if (focusWidget)
	{
		inputMode.SetWidgetToFocus(focusWidget->TakeWidget());
	}
	SetInputMode(inputMode);
	bShowMouseCursor = newMode == ELRInputMode::Menu;
}

/**
 * @brief 查询 LRHUD；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
ALRHUD* ALRPlayerController::GetLRHUD() const
{
	return GetHUD<ALRHUD>();
}
