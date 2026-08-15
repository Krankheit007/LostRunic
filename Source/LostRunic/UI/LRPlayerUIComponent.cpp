/**
 * @file LRPlayerUIComponent.cpp
 * @brief 实现 HUD、状态遮罩、对话/阅读、统一菜单（背包/笔记/收藏）、暂停、存档槽和过场的控制器边界。UI 订阅领域事件并负责表现，不参与核心规则判定。
 *
 * 关联文件：LRPlayerUIComponent.h；所属领域：UI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "UI/LRPlayerUIComponent.h"

#include "Core/LRGameplayTags.h"
#include "Data/LRGameContentSet.h"
#include "Framework/LRCharacter.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "Framework/LRPlayerController.h"
#include "Interaction/LRInteractionComponent.h"
#include "Items/LRItemActionComponent.h"
#include "Narrative/LRDialogueSubsystem.h"
#include "Save/LRGameStatisticsSubsystem.h"
#include "UI/LRDialogueWidgetController.h"
#include "UI/LRHUD.h"
#include "UI/LRScreenWidget.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

/**
 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
 */
ULRPlayerUIComponent::ULRPlayerUIComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

/**
 * @brief 解除委托并清理计时器或缓存，避免关卡切换和对象销毁后继续收到回调。
 * @param endPlayReason Unreal 提供的结束原因，用于区分销毁、关卡切换和退出。
 */
void ULRPlayerUIComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	UnbindNarrative();
	ItemSelectorTarget.Reset();
	bTransitionActive = false;
	bDialogueActive = false;
	bMenuActive = false;
	SetWorldPaused(false);
	OwnerController.Reset();
	Super::EndPlay(endPlayReason);
}

/**
 * @brief 创建并绑定本地玩家 UI；领域状态继续由角色组件和子系统拥有。
 * @param playerController 参与本次操作的运行时对象 `playerController`；函数会检查空值和所需接口。
 */
void ULRPlayerUIComponent::InitializeUI(ALRPlayerController* playerController)
{
	if (!playerController || OwnerController == playerController)
	{
		return;
	}
	UnbindNarrative();
	OwnerController = playerController;
	DialogueSubsystem = playerController->GetGameInstance()->GetSubsystem<ULRDialogueSubsystem>();
	if (ULRDialogueSubsystem* dialogueSubsystem = DialogueSubsystem.Get())
	{
		dialogueSubsystem->OnPageChanged.AddDynamic(this, &ULRPlayerUIComponent::HandleNarrativePageChanged);
		dialogueSubsystem->OnSessionEnded.AddDynamic(this, &ULRPlayerUIComponent::HandleNarrativeSessionEnded);
	}
	if (ALRHUD* hud = GetLRHUD())
	{
		hud->InitializeForController(playerController);
	}
}

/**
 * @brief 更新 Observed Character，并在需要时同步组件状态或广播变化事件。
 * @param character 参与本次操作的运行时对象 `character`；函数会检查空值和所需接口。
 */
void ULRPlayerUIComponent::SetObservedCharacter(ALRCharacter* character)
{
	if (ALRHUD* hud = GetLRHUD())
	{
		hud->SetObservedCharacter(character);
	}
}

/**
 * @brief 处理 Handle Confirm 事件，将引擎回调转换为对应领域状态更新。
 */
void ULRPlayerUIComponent::HandleConfirm()
{
	const ALRPlayerController* controller = OwnerController.Get();
	if (!controller)
	{
		return;
	}
	if (controller->GetLRInputMode() == ELRInputMode::Dialogue)
	{
		if (ALRHUD* hud = GetLRHUD())
		{
			if (ULRDialogueWidgetController* dialogueController = hud->GetDialogueController())
			{
				dialogueController->HandleConfirm();
			}
		}
	}
	else if (controller->GetLRInputMode() == ELRInputMode::Menu)
	{
		if (ULRScreenWidget* screen = GetFocusableScreen())
		{
			screen->HandleUICommand(ELRUICommand::Confirm);
		}
	}
}

/**
 * @brief 处理 Handle Cancel 事件，将引擎回调转换为对应领域状态更新。
 */
void ULRPlayerUIComponent::HandleCancel()
{
	const ALRPlayerController* controller = OwnerController.Get();
	if (!controller)
	{
		return;
	}
	if (controller->GetLRInputMode() == ELRInputMode::Dialogue)
	{
		if (ALRHUD* hud = GetLRHUD())
		{
			if (ULRDialogueWidgetController* dialogueController = hud->GetDialogueController())
			{
				dialogueController->EndSession();
			}
		}
	}
	else if (controller->GetLRInputMode() == ELRInputMode::Menu)
	{
		ULRScreenWidget* screen = GetFocusableScreen();
		if (!screen || !screen->HandleUICommand(ELRUICommand::Cancel))
		{
			CloseMenuScreen();
		}
	}
}

/**
 * @brief 处理 Handle Open Inventory 事件：仅在 Gameplay 模式打开统一菜单背包页；菜单已打开时不处理（不关闭菜单）。
 */
void ULRPlayerUIComponent::HandleOpenInventory()
{
	if (const ALRPlayerController* controller = OwnerController.Get())
	{
		if (controller->GetLRInputMode() == ELRInputMode::Gameplay)
		{
			OpenMenuScreen(ELRScreenType::Inventory);
		}
		// 菜单已打开或其他输入层激活时不处理；I 不加入 Menu Context，因此不能用于关闭菜单。
	}
}

/**
 * @brief 处理 Handle Pause 事件，将引擎回调转换为对应领域状态更新。
 */
void ULRPlayerUIComponent::HandlePause()
{
	if (const ALRPlayerController* controller = OwnerController.Get())
	{
		if (controller->GetLRInputMode() == ELRInputMode::Gameplay)
		{
			OpenMenuScreen(ELRScreenType::Pause);
		}
		else if (controller->GetLRInputMode() == ELRInputMode::Menu)
		{
			CloseMenuScreen();
		}
	}
}

/**
 * @brief 处理 Handle Navigate 事件：方向导航交给当前可聚焦 Screen（Slate Navigation 元数据）。
 * @param direction 本次输入、状态更新或测试使用的值；二维输入只取绝对值较大的轴。
 */
void ULRPlayerUIComponent::HandleNavigate(const FVector2D& direction)
{
	const ALRPlayerController* controller = OwnerController.Get();
	if (!controller || controller->GetLRInputMode() != ELRInputMode::Menu)
	{
		return;
	}
	if (ULRScreenWidget* screen = GetFocusableScreen())
	{
		screen->HandleNavigate(direction);
	}
}

/**
 * @brief 处理 Handle Previous Tab 事件：切换统一菜单上一页。
 */
void ULRPlayerUIComponent::HandlePreviousTab()
{
	const ALRPlayerController* controller = OwnerController.Get();
	if (!controller || controller->GetLRInputMode() != ELRInputMode::Menu)
	{
		return;
	}
	if (ULRScreenWidget* screen = GetFocusableScreen())
	{
		screen->HandleUICommand(ELRUICommand::PreviousTab);
	}
}

/**
 * @brief 处理 Handle Next Tab 事件：切换统一菜单下一页。
 */
void ULRPlayerUIComponent::HandleNextTab()
{
	const ALRPlayerController* controller = OwnerController.Get();
	if (!controller || controller->GetLRInputMode() != ELRInputMode::Menu)
	{
		return;
	}
	if (ULRScreenWidget* screen = GetFocusableScreen())
	{
		screen->HandleUICommand(ELRUICommand::NextTab);
	}
}

/**
 * @brief 处理 Handle UI Primary Action 事件：对当前 UI 条目执行主要业务动作（本界面为装备焦点武器）。
 */
void ULRPlayerUIComponent::HandleUIPrimaryAction()
{
	const ALRPlayerController* controller = OwnerController.Get();
	if (!controller || controller->GetLRInputMode() != ELRInputMode::Menu)
	{
		return;
	}
	if (ULRScreenWidget* screen = GetFocusableScreen())
	{
		screen->HandleUICommand(ELRUICommand::PrimaryAction);
	}
}

/**
 * @brief 打开指定背包、笔记、收藏、暂停或存档页面，并切换到 Menu 输入上下文。
 * @param screen 本次操作使用的 `screen` 枚举或模式值。
 */
void ULRPlayerUIComponent::OpenMenuScreen(const ELRScreenType screen)
{
	ALRPlayerController* controller = OwnerController.Get();
	if (!controller || screen == ELRScreenType::None || GetComputedInputMode() != ELRInputMode::Gameplay)
	{
		return;
	}
	if (ALRHUD* hud = GetLRHUD())
	{
		hud->ShowMenu(screen, true);
	}
	if (screen == ELRScreenType::Pause)
	{
		SetWorldPaused(true);
	}
	bMenuActive = true;
	ApplyArbitratedInputMode();
}

/**
 * @brief 为需要物品的交互目标打开统一菜单的背包 Tab（交互选物模式）；只有与目标兼容的物品可提交。
 * @param target 当前已通过交互筛选的物品使用目标。
 */
void ULRPlayerUIComponent::OpenItemSelector(AActor* target)
{
	ALRPlayerController* controller = OwnerController.Get();
	if (!controller || !target || GetComputedInputMode() != ELRInputMode::Gameplay)
	{
		return;
	}
	ItemSelectorTarget = target;
	OpenMenuScreen(ELRScreenType::Inventory);
}

/**
 * @brief 关闭当前菜单层并恢复仍然有效的下层输入上下文，同时抑制切换时仍按住的按键。
 */
void ULRPlayerUIComponent::CloseMenuScreen()
{
	if (!bMenuActive)
	{
		return;
	}
	if (ALRHUD* hud = GetLRHUD())
	{
		hud->ShowMenu(ELRScreenType::None, false);
	}
	ItemSelectorTarget.Reset();
	bMenuActive = false;
	SetWorldPaused(false);
	ApplyArbitratedInputMode();
}

void ULRPlayerUIComponent::SetWorldPaused(const bool bPaused)
{
	if (bWorldPaused == bPaused)
	{
		return;
	}
	if (UWorld* world = GetWorld())
	{
		UGameplayStatics::SetGamePaused(world, bPaused);
	}
	bWorldPaused = bPaused;

	UGameInstance* gameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	ULRGameStatisticsSubsystem* statistics = gameInstance
		? gameInstance->GetSubsystem<ULRGameStatisticsSubsystem>() : nullptr;
	if (!statistics)
	{
		return;
	}
	bool bResumePlayTime = false;
	if (!bPaused)
	{
		const ULRGameInstanceSubsystem* data = gameInstance->GetSubsystem<ULRGameInstanceSubsystem>();
		const ULRGameContentSet* content = data ? data->GetContentSet() : nullptr;
		const FLRMapRegistration* map = content
			? content->FindMapRegistration(content->FindMapIdForWorld(GetWorld())) : nullptr;
		bResumePlayTime = map && map->bPlayableMap;
	}
	statistics->SetPlayTimeActive(bResumePlayTime);
}

/**
 * @brief 执行 Use Inventory Item 的玩法动作；输入层只提供语义，合法性由统一物品事务决定。
 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FLRItemUseResult ULRPlayerUIComponent::UseInventoryItem(const FName itemId) const
{
	const ALRPlayerController* controller = OwnerController.Get();
	const ALRCharacter* character = controller ? Cast<ALRCharacter>(controller->GetPawn()) : nullptr;
	if (!character)
	{
		return FLRItemUseResult();
	}
	AActor* target = ItemSelectorTarget.IsValid() ? ItemSelectorTarget.Get()
		: character->GetInteractionComponent()->GetCurrentTarget();
	return character->GetItemActionComponent()->RequestUseItem(itemId, target);
}

/**
 * @brief 启用或关闭 Transition 输入层；关闭后恢复仍然有效的下层（Dialogue > Menu > Gameplay）。
 * @param bActive 布尔开关 `bActive`；true 表示启用或条件成立，false 表示禁用或条件不成立。
 */
void ULRPlayerUIComponent::SetTransitionLayer(const bool bActive)
{
	if (bTransitionActive == bActive)
	{
		return;
	}
	bTransitionActive = bActive;
	ApplyArbitratedInputMode();
}

/**
 * @brief 启用或关闭 Dialogue 输入层；关闭后恢复仍然有效的下层（Menu > Gameplay）。
 * @param bActive 布尔开关 `bActive`；true 表示启用或条件成立，false 表示禁用或条件不成立。
 */
void ULRPlayerUIComponent::SetDialogueLayer(const bool bActive)
{
	if (bDialogueActive == bActive)
	{
		return;
	}
	bDialogueActive = bActive;
	ApplyArbitratedInputMode();
}

/**
 * @brief 启用或关闭 Menu 输入层；关闭后恢复仍然有效的下层（Gameplay）。
 * @param bActive 布尔开关 `bActive`；true 表示启用或条件成立，false 表示禁用或条件不成立。
 */
void ULRPlayerUIComponent::SetMenuLayer(const bool bActive)
{
	if (bMenuActive == bActive)
	{
		return;
	}
	bMenuActive = bActive;
	ApplyArbitratedInputMode();
}

/**
 * @brief 按 Transition > Dialogue > Menu > Gameplay 计算唯一有效输入层。
 * @param bTransitionActive 布尔开关 `bTransitionActive`；true 表示启用或条件成立，false 表示禁用或条件不成立。
 * @param bDialogueActive 布尔开关 `bDialogueActive`；true 表示启用或条件成立，false 表示禁用或条件不成立。
 * @param bMenuActive 布尔开关 `bMenuActive`；true 表示启用或条件成立，false 表示禁用或条件不成立。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
ELRInputMode ULRPlayerUIComponent::ComputeEffectiveInputMode(const bool bTransitionActive, const bool bDialogueActive, const bool bMenuActive)
{
	if (bTransitionActive)
	{
		return ELRInputMode::Transition;
	}
	if (bDialogueActive)
	{
		return ELRInputMode::Dialogue;
	}
	if (bMenuActive)
	{
		return ELRInputMode::Menu;
	}
	return ELRInputMode::Gameplay;
}

/**
 * @brief 查询当前仲裁结果；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
ELRInputMode ULRPlayerUIComponent::GetComputedInputMode() const
{
	return ComputeEffectiveInputMode(bTransitionActive, bDialogueActive, bMenuActive);
}

/**
 * @brief 把仲裁出的唯一输入层应用到 Controller；只在值变化时调用 SetLRInputMode。
 */
void ULRPlayerUIComponent::ApplyArbitratedInputMode()
{
	ALRPlayerController* controller = OwnerController.Get();
	if (!controller)
	{
		return;
	}
	const ELRInputMode effective = GetComputedInputMode();
	if (controller->GetLRInputMode() != effective)
	{
		controller->SetLRInputMode(effective);
	}
}

/**
 * @brief 把内部失败原因标签映射为面向玩家的友好提示，不暴露内部 Tag。
 * @param failureReason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FText ULRPlayerUIComponent::DescribeItemUseFailure(const FGameplayTag failureReason) const
{
	if (failureReason == LRGameplayTags::InteractionRejectItem
		|| failureReason == LRGameplayTags::ItemUseRejectInvalidAttackItem)
	{
		return NSLOCTEXT("LRMainMenu", "Incompatible", "This item cannot be used here.");
	}
	if (failureReason == LRGameplayTags::ItemUseRejectInventoryFull)
	{
		return NSLOCTEXT("LRMainMenu", "InventoryFull", "Inventory is full!");
	}
	if (failureReason == LRGameplayTags::ItemUseRejectTarget)
	{
		return NSLOCTEXT("LRMainMenu", "TargetUnavailable", "The target is no longer available.");
	}
	if (failureReason == LRGameplayTags::ItemUseRejectAttackState || failureReason == LRGameplayTags::StateRejectBlocked)
	{
		return NSLOCTEXT("LRMainMenu", "AttackUnavailable", "You cannot attack right now.");
	}
	if (failureReason == LRGameplayTags::CollectibleRejectAlreadyOwned)
	{
		return NSLOCTEXT("LRMainMenu", "AlreadyOwned", "You already own this collectible.");
	}
	return NSLOCTEXT("LRMainMenu", "UseFailed", "The item could not be used.");
}

/**
 * @brief 处理 Handle Narrative Page Changed 事件，将引擎回调转换为对应领域状态更新。
 * @param page 本次领域操作的结构化数据 `page`；字段语义由对应 USTRUCT 定义。
 */
void ULRPlayerUIComponent::HandleNarrativePageChanged(const FLRNarrativePage page)
{
	if (ALRHUD* hud = GetLRHUD())
	{
		hud->ShowNarrative(true);
	}
	bDialogueActive = true;
	ApplyArbitratedInputMode();
}

/**
 * @brief 处理 Handle Narrative Session Ended 事件，将引擎回调转换为对应领域状态更新。
 * @param sessionType 本次操作使用的 `sessionType` 枚举或模式值。
 * @param finalContentId 稳定标识 `finalContentId`；用于内容查询和存档，不依赖显示名或数组序号。
 */
void ULRPlayerUIComponent::HandleNarrativeSessionEnded(const ELRNarrativeSessionType sessionType, const FName finalContentId)
{
	if (ALRHUD* hud = GetLRHUD())
	{
		hud->ShowNarrative(false);
	}
	bDialogueActive = false;
	ApplyArbitratedInputMode();
}

/**
 * @brief 查询 LRHUD；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
ALRHUD* ULRPlayerUIComponent::GetLRHUD() const
{
	const ALRPlayerController* controller = OwnerController.Get();
	return controller ? controller->GetHUD<ALRHUD>() : nullptr;
}

/**
 * @brief 查询当前输入层对应的可聚焦 Screen；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
ULRScreenWidget* ULRPlayerUIComponent::GetFocusableScreen() const
{
	const ALRHUD* hud = GetLRHUD();
	const ALRPlayerController* controller = OwnerController.Get();
	return hud && controller ? hud->GetFocusableScreen(controller->GetLRInputMode()) : nullptr;
}

/**
 * @brief 解除 UI 对叙事子系统的委托绑定，避免销毁或换图后重复回调。
 */
void ULRPlayerUIComponent::UnbindNarrative()
{
	if (ULRDialogueSubsystem* dialogueSubsystem = DialogueSubsystem.Get())
	{
		dialogueSubsystem->OnPageChanged.RemoveDynamic(this, &ULRPlayerUIComponent::HandleNarrativePageChanged);
		dialogueSubsystem->OnSessionEnded.RemoveDynamic(this, &ULRPlayerUIComponent::HandleNarrativeSessionEnded);
	}
	DialogueSubsystem.Reset();
}
