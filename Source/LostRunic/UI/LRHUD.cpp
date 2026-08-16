/**
 * @file LRHUD.cpp
 * @brief 创建并管理独立 HUD、状态遮罩、对话、阅读、背包、笔记、收藏、暂停、存档槽与过场 Widget，不在根 Widget 内集中核心逻辑。
 *
 * 关联文件：LRHUD.h；所属领域：UI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "UI/LRHUD.h"

#include "Core/LRLog.h"
#include "Data/LRGameTuningSet.h"
#include "Data/LRProjectSettings.h"
#include "Framework/LRCharacter.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "Framework/LRPlayerController.h"
#include "Framework/LRGameFlowSubsystem.h"
#include "Narrative/LRDialogueSubsystem.h"
#include "UI/LRDialogueWidgetController.h"
#include "UI/LRHUDWidgetController.h"
#include "UI/LRInventoryScreenWidget.h"
#include "UI/LRPauseWidget.h"
#include "UI/LRPlayerUIComponent.h"
#include "UI/LRSaveSelectionWidget.h"
#include "UI/LRMenuWidgetController.h"
#include "UI/LRScreenWidget.h"
#include "UI/LRSaveWidgetController.h"
#include "UI/LRTransitionWidgetController.h"
#include "Save/LRSaveSubsystem.h"

/**
 * @brief 解除委托并清理计时器或缓存，避免关卡切换和对象销毁后继续收到回调。
 * @param endPlayReason Unreal 提供的结束原因，用于区分销毁、关卡切换和退出。
 */
void ALRHUD::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	if (DialogueController)
	{
		DialogueController->Deinitialize();
	}
	if (HUDController)
	{
		HUDController->Deinitialize();
	}
	if (SaveController)
	{
		SaveController->Deinitialize();
	}
	Super::EndPlay(endPlayReason);
}

/**
 * @brief 为本地 PlayerController 创建控制器对象、绑定角色及叙事事件，并建立初始 HUD。
 * @param playerController 参与本次操作的运行时对象 `playerController`；函数会检查空值和所需接口。
 */
void ALRHUD::InitializeForController(ALRPlayerController* playerController)
{
	if (!playerController)
	{
		return;
	}
	ULRGameInstanceSubsystem* dataSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	ULRGameTuningSet* tuningSet = dataSubsystem ? dataSubsystem->GetTuningSet() : nullptr;
	if (!DialogueController)
	{
		DialogueController = NewObject<ULRDialogueWidgetController>(this);
		ULRDialogueSubsystem* dialogueSubsystem = GetGameInstance()->GetSubsystem<ULRDialogueSubsystem>();
		DialogueController->Initialize(dialogueSubsystem, tuningSet ? tuningSet->UI : nullptr, GetWorld());
		DialogueController->OnPresentationChanged.AddDynamic(this, &ALRHUD::HandleNarrativePresentationChanged);
	}
	if (!HUDController)
	{
		HUDController = NewObject<ULRHUDWidgetController>(this);
	}
	if (!MenuController)
	{
		MenuController = NewObject<ULRMenuWidgetController>(this);
		MenuController->OnMenuScreenChanged.AddDynamic(this, &ALRHUD::HandleMenuScreenChanged);
	}
	if (!TransitionController)
	{
		TransitionController = NewObject<ULRTransitionWidgetController>(this);
		TransitionController->OnTransitionVisibilityChanged.AddDynamic(this, &ALRHUD::HandleTransitionVisibilityChanged);
	}
	if (!SaveController)
	{
		SaveController = NewObject<ULRSaveWidgetController>(this);
		SaveController->Initialize(GetGameInstance()->GetSubsystem<ULRSaveSubsystem>(), GetContentSet());
	}
	// 菜单控制器绑定库存与内容定义；Possess 尚未发生时在此尽力绑定，SetObservedCharacter 会兜底。
	BindMenuControllerToCharacter(playerController);
	if (!HUDScreenClass && ShouldUseProjectDefaultHUDScreen())
	{
		HUDScreenClass = GetDefault<ULRProjectSettings>()->HUDScreenClass.LoadSynchronous();
	}
	if (!HUDScreenClass && ShouldUseProjectDefaultHUDScreen())
	{
		UE_LOG(LogLostRunicUI, Warning, TEXT("HUD=%s has no HUDScreenClass; no HUD screen will be created."), *GetNameSafe(this));
	}
	CreateScreens(playerController);
	// 统一菜单 Screen 注入菜单控制器：页面变化与快照变化直接由 Widget 消费。
	if (ULRInventoryScreenWidget* inventoryScreen = Cast<ULRInventoryScreenWidget>(GetScreen(ELRScreenType::Inventory)))
	{
		inventoryScreen->SetMenuWidgetController(MenuController);
	}
	if (ULRScreenWidget* saveScreen = GetScreen(ELRScreenType::SaveSlots))
	{
		saveScreen->SetSaveWidgetController(SaveController);
	}
	if (ULRPauseWidget* pauseScreen = Cast<ULRPauseWidget>(GetScreen(ELRScreenType::Pause)))
	{
		pauseScreen->OnResumeRequested.AddDynamic(this, &ALRHUD::HandlePauseResumeRequested);
		pauseScreen->OnSaveRequested.AddDynamic(this, &ALRHUD::HandlePauseSaveRequested);
		pauseScreen->OnMainMenuRequested.AddDynamic(this, &ALRHUD::HandlePauseMainMenuRequested);
	}
	if (ULRSaveSelectionWidget* saveScreen = Cast<ULRSaveSelectionWidget>(GetScreen(ELRScreenType::SaveSlots)))
	{
		saveScreen->OnBackRequested.AddDynamic(this, &ALRHUD::HandleSaveSelectionBackRequested);
	}
	SetObservedCharacter(Cast<ALRCharacter>(playerController->GetPawn()));
	SetScreenVisible(ELRScreenType::HUD, true);
	SetScreenVisible(ELRScreenType::StateOverlay, true);
}

/**
 * @brief 更新 Observed Character，并在需要时同步组件状态或广播变化事件。
 * @param character 参与本次操作的运行时对象 `character`；函数会检查空值和所需接口。
 */
void ALRHUD::SetObservedCharacter(ALRCharacter* character)
{
	if (HUDController)
	{
		HUDController->SetObservedCharacter(character);
	}
	if (MenuController && character && !MenuController->GetInventory())
	{
		MenuController->Initialize(character->GetInventoryComponent(), GetContentSet(), GetUITuning());
	}
}

/**
 * @brief 尽力把菜单控制器绑定到角色库存；Possess 未发生时 Inventory 为空，SetObservedCharacter 会兜底完成绑定。
 * @param playerController 参与本次操作的运行时对象 `playerController`；函数会检查空值和所需接口。
 */
void ALRHUD::BindMenuControllerToCharacter(ALRPlayerController* playerController)
{
	if (!MenuController || MenuController->GetInventory())
	{
		return;
	}
	if (ALRCharacter* character = Cast<ALRCharacter>(playerController ? playerController->GetPawn() : nullptr))
	{
		MenuController->Initialize(character->GetInventoryComponent(), GetContentSet(), GetUITuning());
	}
}

/**
 * @brief 查询当前内容集；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
ULRGameContentSet* ALRHUD::GetContentSet() const
{
	const ULRGameInstanceSubsystem* dataSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	return dataSubsystem ? dataSubsystem->GetContentSet() : nullptr;
}

/**
 * @brief 查询当前 UI 调优；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
ULRUITuning* ALRHUD::GetUITuning() const
{
	const ULRGameInstanceSubsystem* dataSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	const ULRGameTuningSet* tuningSet = dataSubsystem ? dataSubsystem->GetTuningSet() : nullptr;
	return tuningSet ? tuningSet->UI : nullptr;
}

/**
 * @brief 显示或隐藏对话/阅读层；具体文本来自叙事控制器。
 * @param bVisible 布尔开关 `bVisible`；true 表示启用或条件成立，false 表示禁用或条件不成立。
 */
void ALRHUD::ShowNarrative(const bool bVisible)
{
	SetScreenVisible(ELRScreenType::Narrative, bVisible);
}

/**
 * @brief 显示或隐藏指定菜单 Widget，并维护唯一可聚焦页面。
 * @param screen 本次操作使用的 `screen` 枚举或模式值。
 * @param bVisible 布尔开关 `bVisible`；true 表示启用或条件成立，false 表示禁用或条件不成立。
 */
void ALRHUD::ShowMenu(const ELRScreenType screen, const bool bVisible)
{
	if (!MenuController)
	{
		return;
	}
	if (bVisible && MenuController->OpenScreen(screen))
	{
		return;
	}
	if (!bVisible)
	{
		MenuController->CloseScreen();
	}
}

void ALRHUD::OpenSaveSelection(const ELRSaveSelectionMode mode)
{
	PendingSaveSelectionMode = mode;
	if (MenuController)
	{
		MenuController->OpenScreen(ELRScreenType::SaveSlots);
	}
}

void ALRHUD::ReturnFromSaveSelection()
{
	if (MenuController)
	{
		MenuController->OpenScreen(ELRScreenType::Pause);
	}
}

/**
 * @brief 显示或隐藏过场遮罩，并同步 Transition 输入阻塞。
 * @param bVisible 布尔开关 `bVisible`；true 表示启用或条件成立，false 表示禁用或条件不成立。
 */
void ALRHUD::ShowTransition(const bool bVisible)
{
	if (TransitionController)
	{
		TransitionController->SetTransitionVisible(bVisible);
	}
}

/**
 * @brief 查询 Screen；不修改领域状态。
 * @param screen 本次操作使用的 `screen` 枚举或模式值。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
ULRScreenWidget* ALRHUD::GetScreen(const ELRScreenType screen) const
{
	const TObjectPtr<ULRScreenWidget>* found = ScreenWidgets.Find(screen);
	return found ? found->Get() : nullptr;
}

/**
 * @brief 查询 Focusable Screen；不修改领域状态。
 * @param inputMode 本次操作使用的 `inputMode` 枚举或模式值。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
ULRScreenWidget* ALRHUD::GetFocusableScreen(const ELRInputMode inputMode) const
{
	if (inputMode == ELRInputMode::Dialogue)
	{
		return GetScreen(ELRScreenType::Narrative);
	}
	if (inputMode == ELRInputMode::Menu && MenuController)
	{
		const ELRScreenType openScreen = MenuController->GetOpenScreen();
		if (openScreen == ELRScreenType::Pause || openScreen == ELRScreenType::SaveSlots)
		{
			return GetScreen(openScreen);
		}
		return GetScreen(ELRScreenType::Inventory);
	}
	if (inputMode == ELRInputMode::Transition)
	{
		return GetScreen(ELRScreenType::Transition);
	}
	return nullptr;
}

/**
 * @brief 根据当前领域状态构建 Create Screens 所需的数据，不把临时对象作为长期存档标识。
 * @param playerController 参与本次操作的运行时对象 `playerController`；函数会检查空值和所需接口。
 */
void ALRHUD::CreateScreens(ALRPlayerController* playerController)
{
	if (!ScreenWidgets.IsEmpty())
	{
		return;
	}
	CreateScreen(playerController, ELRScreenType::HUD, HUDScreenClass);
	CreateScreen(playerController, ELRScreenType::StateOverlay, StateOverlayScreenClass);
	CreateScreen(playerController, ELRScreenType::Narrative, NarrativeScreenClass);
	CreateScreen(playerController, ELRScreenType::Inventory, MenuScreenClass);
	CreateScreen(playerController, ELRScreenType::Pause, PauseScreenClass);
	CreateScreen(playerController, ELRScreenType::SaveSlots, SaveSlotsScreenClass);
	CreateScreen(playerController, ELRScreenType::Transition, TransitionScreenClass);
}

/**
 * @brief 根据当前领域状态构建 Create Screen 所需的数据，不把临时对象作为长期存档标识。
 * @param playerController 参与本次操作的运行时对象 `playerController`；函数会检查空值和所需接口。
 * @param screen 本次操作使用的 `screen` 枚举或模式值。
 * @param screenClass 调用方提供的 `screenClass`，只在本次操作范围内使用。
 */
void ALRHUD::CreateScreen(ALRPlayerController* playerController, const ELRScreenType screen,
	const TSubclassOf<ULRScreenWidget> screenClass)
{
	if (!screenClass)
	{
		return;
	}
	ULRScreenWidget* widget = CreateWidget<ULRScreenWidget>(playerController, screenClass);
	if (widget)
	{
		widget->AddToPlayerScreen();
		widget->SetHUDWidgetController(HUDController);
		widget->SetScreenVisible(false);
		ScreenWidgets.Add(screen, widget);
	}
}

/**
 * @brief 更新 Screen Visible，并在需要时同步组件状态或广播变化事件。
 * @param screen 本次操作使用的 `screen` 枚举或模式值。
 * @param bVisible 布尔开关 `bVisible`；true 表示启用或条件成立，false 表示禁用或条件不成立。
 */
void ALRHUD::SetScreenVisible(const ELRScreenType screen, const bool bVisible)
{
	if (ULRScreenWidget* widget = GetScreen(screen))
	{
		widget->SetScreenVisible(bVisible);
	}
}

/**
 * @brief 隐藏所有互斥菜单页面，确保同一时刻只有一个焦点目标。
 */
void ALRHUD::HideMenuScreens()
{
	SetScreenVisible(ELRScreenType::Inventory, false);
	SetScreenVisible(ELRScreenType::Pause, false);
	SetScreenVisible(ELRScreenType::SaveSlots, false);
}

/**
 * @brief 处理统一菜单 Tab 切换：显示统一菜单 Widget 并调用 Inventory Screen 的 SetActiveTab。
 * @param tab 本次操作使用的 `tab` 枚举或模式值。
 */
void ALRHUD::ShowMenuTab(const ELRScreenType tab)
{
	if (ULRInventoryScreenWidget* inventoryScreen = Cast<ULRInventoryScreenWidget>(GetScreen(ELRScreenType::Inventory)))
	{
		inventoryScreen->SetScreenVisible(true);
		inventoryScreen->SetActiveTab(tab);
	}
}

/**
 * @brief 处理 Handle Narrative Presentation Changed 事件，将引擎回调转换为对应领域状态更新。
 * @param presentation 本次领域操作的结构化数据 `presentation`；字段语义由对应 USTRUCT 定义。
 */
void ALRHUD::HandleNarrativePresentationChanged(const FLRNarrativePresentation presentation)
{
	if (ULRScreenWidget* widget = GetScreen(ELRScreenType::Narrative))
	{
		widget->PresentNarrative(presentation);
	}
}

/**
 * @brief 处理 Handle Menu Screen Changed 事件，将引擎回调转换为对应领域状态更新。
 * @param previousScreen 本次操作使用的 `previousScreen` 枚举或模式值。
 * @param currentScreen 本次操作使用的 `currentScreen` 枚举或模式值。
 */
void ALRHUD::HandleMenuScreenChanged(const ELRScreenType previousScreen, const ELRScreenType currentScreen)
{
	if (previousScreen == ELRScreenType::SaveSlots && currentScreen != ELRScreenType::SaveSlots && SaveController)
	{
		SaveController->Close();
	}
	HideMenuScreens();
	if (currentScreen == ELRScreenType::None)
	{
		return;
	}
	if (currentScreen == ELRScreenType::Pause || currentScreen == ELRScreenType::SaveSlots)
	{
		if (currentScreen == ELRScreenType::SaveSlots && SaveController)
		{
			SaveController->Open(PendingSaveSelectionMode);
		}
		SetScreenVisible(currentScreen, true);
		return;
	}
	ShowMenuTab(currentScreen);
}

void ALRHUD::HandlePauseResumeRequested()
{
	ALRPlayerController* controller = Cast<ALRPlayerController>(PlayerOwner);
	if (controller && controller->GetPlayerUI())
	{
		controller->GetPlayerUI()->CloseMenuScreen();
	}
}

void ALRHUD::HandlePauseSaveRequested()
{
	OpenSaveSelection(ELRSaveSelectionMode::Save);
}

void ALRHUD::HandlePauseMainMenuRequested()
{
	ALRPlayerController* controller = Cast<ALRPlayerController>(PlayerOwner);
	if (controller && controller->GetPlayerUI())
	{
		controller->GetPlayerUI()->CloseMenuScreen();
	}
	if (ULRGameFlowSubsystem* flow = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULRGameFlowSubsystem>() : nullptr)
	{
		flow->TravelToMainMenu();
	}
}

void ALRHUD::HandleSaveSelectionBackRequested()
{
	ReturnFromSaveSelection();
}

/**
 * @brief 处理 Handle Transition Visibility Changed 事件，将引擎回调转换为对应领域状态更新。
 * @param bVisible 布尔开关 `bVisible`；true 表示启用或条件成立，false 表示禁用或条件不成立。
 */
void ALRHUD::HandleTransitionVisibilityChanged(const bool bVisible)
{
	SetScreenVisible(ELRScreenType::Transition, bVisible);
}
