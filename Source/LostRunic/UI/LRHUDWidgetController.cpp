/**
 * @file LRHUDWidgetController.cpp
 * @brief 实现 HUD、状态遮罩、对话/阅读、背包/笔记/收藏、暂停、存档槽和过场的控制器边界。UI 订阅领域事件并负责表现，不参与核心规则判定。
 *
 * 关联文件：LRHUDWidgetController.h；所属领域：UI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "UI/LRHUDWidgetController.h"

#include "Framework/LRCharacter.h"
#include "Interaction/LRInteractionComponent.h"
#include "State/LRStateComponent.h"

/**
 * @brief 更新 Observed Character，并在需要时同步组件状态或广播变化事件。
 * @param character 参与本次操作的运行时对象 `character`；函数会检查空值和所需接口。
 */
void ULRHUDWidgetController::SetObservedCharacter(ALRCharacter* character)
{
	Deinitialize();
	ObservedCharacter = character;
	ULRStateComponent* state = character ? character->GetStateComponent() : nullptr;
	if (!state)
	{
		return;
	}
	CurrentMode = state->GetCurrentMode();
	state->OnStateChanged.AddDynamic(this, &ULRHUDWidgetController::HandleStateChanged);
	if (ULRInteractionComponent* interaction = character->GetInteractionComponent())
	{
		CurrentInteractionPrompt = interaction->GetFocusedPrompt();
		interaction->OnFocusedInteractionChanged.AddDynamic(this, &ULRHUDWidgetController::HandleFocusedInteractionChanged);
		OnInteractionPromptChanged.Broadcast(CurrentInteractionPrompt);
	}
}

/**
 * @brief 释放子系统事件绑定和运行时缓存。
 */
void ULRHUDWidgetController::Deinitialize()
{
	if (ALRCharacter* character = ObservedCharacter.Get())
	{
		character->GetStateComponent()->OnStateChanged.RemoveDynamic(this, &ULRHUDWidgetController::HandleStateChanged);
		if (ULRInteractionComponent* interaction = character->GetInteractionComponent())
		{
			interaction->OnFocusedInteractionChanged.RemoveDynamic(this, &ULRHUDWidgetController::HandleFocusedInteractionChanged);
		}
	}
	ObservedCharacter.Reset();
	CurrentMode = ELRPerceptionMode::Normal;
	CurrentInteractionPrompt = FLRInteractionPromptView();
}

/**
 * @brief 处理 Handle State Changed 事件，将引擎回调转换为对应领域状态更新。
 * @param currentMode 本次操作使用的 `currentMode` 枚举或模式值。
 * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
 */
void ULRHUDWidgetController::HandleStateChanged(const ELRPerceptionMode currentMode, const FGameplayTag reason)
{
	CurrentMode = currentMode;
	OnPerceptionModeChanged.Broadcast(CurrentMode, reason);
}

/** Stores and forwards the Focus prompt produced by the interaction component. */
void ULRHUDWidgetController::HandleFocusedInteractionChanged(const FLRInteractionPromptView promptView)
{
	CurrentInteractionPrompt = promptView;
	OnInteractionPromptChanged.Broadcast(CurrentInteractionPrompt);
}
