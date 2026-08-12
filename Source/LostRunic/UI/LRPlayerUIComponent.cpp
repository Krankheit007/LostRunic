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
#include "Framework/LRCharacter.h"
#include "Framework/LRPlayerController.h"
#include "Items/LRItemActionComponent.h"
#include "Narrative/LRDialogueSubsystem.h"
#include "UI/LRDialogueWidgetController.h"
#include "UI/LRHUD.h"
#include "UI/LRScreenWidget.h"

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
		CloseMenuScreen();
	}
}

/**
 * @brief 处理 Handle Open Journal 事件，将引擎回调转换为对应领域状态更新。
 */
void ULRPlayerUIComponent::HandleOpenJournal()
{
	if (const ALRPlayerController* controller = OwnerController.Get())
	{
		if (controller->GetLRInputMode() == ELRInputMode::Gameplay)
		{
			OpenMenuScreen(ELRScreenType::Journal);
		}
		else if (controller->GetLRInputMode() == ELRInputMode::Menu)
		{
			CloseMenuScreen();
		}
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
 * @brief 打开指定背包、笔记、收藏、暂停或存档页面，并切换到 Menu 输入上下文。
 * @param screen 本次操作使用的 `screen` 枚举或模式值。
 */
void ULRPlayerUIComponent::OpenMenuScreen(const ELRScreenType screen)
{
	ALRPlayerController* controller = OwnerController.Get();
	if (!controller || controller->GetLRInputMode() == ELRInputMode::Dialogue || screen == ELRScreenType::None)
	{
		return;
	}
	if (ALRHUD* hud = GetLRHUD())
	{
		hud->ShowMenu(screen, true);
		controller->SetLRInputMode(ELRInputMode::Menu);
	}
}

/**
 * @brief 为需要物品的交互目标打开统一菜单的背包 Tab（交互选物模式）；只有与目标兼容的物品可提交。
 * @param target 当前已通过交互筛选的物品使用目标。
 */
void ULRPlayerUIComponent::OpenItemSelector(AActor* target)
{
	ALRPlayerController* controller = OwnerController.Get();
	if (!controller || !target || controller->GetLRInputMode() != ELRInputMode::Gameplay)
	{
		return;
	}
	ItemSelectorTarget = target;
	OpenMenuScreen(ELRScreenType::Inventory);
}

/**
 * @brief 关闭当前菜单层并恢复 Gameplay 输入上下文，同时抑制切换时仍按住的按键。
 */
void ULRPlayerUIComponent::CloseMenuScreen()
{
	ALRPlayerController* controller = OwnerController.Get();
	if (!controller || controller->GetLRInputMode() != ELRInputMode::Menu)
	{
		return;
	}
	if (ALRHUD* hud = GetLRHUD())
	{
		hud->ShowMenu(ELRScreenType::None, false);
	}
	ItemSelectorTarget.Reset();
	controller->SetLRInputMode(ELRInputMode::Gameplay);
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
	if (ALRPlayerController* controller = OwnerController.Get())
	{
		controller->SetLRInputMode(ELRInputMode::Dialogue);
	}
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
	if (ALRPlayerController* controller = OwnerController.Get(); controller && controller->GetLRInputMode() == ELRInputMode::Dialogue)
	{
		controller->SetLRInputMode(ELRInputMode::Gameplay);
	}
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
