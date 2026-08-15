/**
 * @file LRHUD.h
 * @brief 创建并管理独立 HUD、状态遮罩、对话、阅读、背包、笔记、收藏、暂停、存档槽与过场 Widget，不在根 Widget 内集中核心逻辑。
 *
 * 关联文件：LRHUD.cpp；所属领域：UI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "GameFramework/HUD.h"
#include "UI/LRUITypes.h"

#include "LRHUD.generated.h"

class ALRCharacter;
class ALRPlayerController;
class ULRDialogueWidgetController;
class ULRHUDWidgetController;
class ULRMenuWidgetController;
class ULRScreenWidget;
class ULRSaveWidgetController;
class ULRTransitionWidgetController;

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic HUD"))
class LOSTRUNIC_API ALRHUD : public AHUD
{
	GENERATED_BODY()

public:
	/**
	 * @brief 解除委托并清理计时器或缓存，避免关卡切换和对象销毁后继续收到回调。
	 * @param endPlayReason Unreal 提供的结束原因，用于区分销毁、关卡切换和退出。
	 */
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

	/**
	 * @brief 为本地 PlayerController 创建控制器对象、绑定角色及叙事事件，并建立初始 HUD。
	 * @param playerController 参与本次操作的运行时对象 `playerController`；函数会检查空值和所需接口。
	 */
	void InitializeForController(ALRPlayerController* playerController);
	/**
	 * @brief 更新 Observed Character，并在需要时同步组件状态或广播变化事件。
	 * @param character 参与本次操作的运行时对象 `character`；函数会检查空值和所需接口。
	 */
	void SetObservedCharacter(ALRCharacter* character);
	/**
	 * @brief 显示或隐藏对话/阅读层；具体文本来自叙事控制器。
	 * @param bVisible 布尔开关 `bVisible`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 */
	void ShowNarrative(bool bVisible);
	/**
	 * @brief 显示或隐藏指定菜单 Widget，并维护唯一可聚焦页面。
	 * @param screen 本次操作使用的 `screen` 枚举或模式值。
	 * @param bVisible 布尔开关 `bVisible`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 */
	void ShowMenu(ELRScreenType screen, bool bVisible);
	/**
	 * @brief 显示或隐藏过场遮罩，并同步 Transition 输入阻塞。
	 * @param bVisible 布尔开关 `bVisible`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 */
	void ShowTransition(bool bVisible);

	/**
	 * @brief 查询 Screen；不修改领域状态。
	 * @param screen 本次操作使用的 `screen` 枚举或模式值。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	ULRScreenWidget* GetScreen(ELRScreenType screen) const;
	/**
	 * @brief 查询 Focusable Screen；不修改领域状态。
	 * @param inputMode 本次操作使用的 `inputMode` 枚举或模式值。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	ULRScreenWidget* GetFocusableScreen(ELRInputMode inputMode) const;
	/**
	 * @brief 查询 Dialogue Controller；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	ULRDialogueWidgetController* GetDialogueController() const { return DialogueController; }
	/** Exposes the HUD event source used by the interaction prompt widget. */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|UI")
	ULRHUDWidgetController* GetHUDWidgetController() const { return HUDController; }
	/**
	 * @brief 查询 Menu Controller；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	ULRMenuWidgetController* GetMenuController() const { return MenuController; }
	/**
	 * @brief 查询 Transition Controller；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	ULRTransitionWidgetController* GetTransitionController() const { return TransitionController; }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Save UI")
	ULRSaveWidgetController* GetSaveWidgetController() const { return SaveController; }

	/**
	 * @brief 查询统一菜单 Screen 类；权威配置在 HUD 蓝图默认值，自动化测试用于断言其派生自 ULRInventoryScreenWidget。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	TSubclassOf<ULRScreenWidget> GetMenuScreenClass() const { return MenuScreenClass; }

protected:
	/** HUDScreen Class 的软类或类默认引用，用于创建对应蓝图实例。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, Category = "Widgets")
	TSubclassOf<ULRScreenWidget> HUDScreenClass;

	/** State Overlay Screen Class 的软类或类默认引用，用于创建对应蓝图实例。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, Category = "Widgets")
	TSubclassOf<ULRScreenWidget> StateOverlayScreenClass;

	/** Narrative Screen Class 的软类或类默认引用，用于创建对应蓝图实例。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, Category = "Widgets")
	TSubclassOf<ULRScreenWidget> NarrativeScreenClass;

	/** Menu Screen Class 的软类或类默认引用：背包/笔记/收集品共用一个 UMG 菜单资产，通过顶部 Tab 切换。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, Category = "Widgets")
	TSubclassOf<ULRScreenWidget> MenuScreenClass;

	/** Pause Screen Class 的软类或类默认引用，用于创建对应蓝图实例。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, Category = "Widgets")
	TSubclassOf<ULRScreenWidget> PauseScreenClass;

	/** Save Slots Screen Class 的软类或类默认引用，用于创建对应蓝图实例。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, Category = "Widgets")
	TSubclassOf<ULRScreenWidget> SaveSlotsScreenClass;

	/** Transition Screen Class 的软类或类默认引用，用于创建对应蓝图实例。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, Category = "Widgets")
	TSubclassOf<ULRScreenWidget> TransitionScreenClass;

private:
	/**
	 * @brief 根据当前领域状态构建 Create Screens 所需的数据，不把临时对象作为长期存档标识。
	 * @param playerController 参与本次操作的运行时对象 `playerController`；函数会检查空值和所需接口。
	 */
	void CreateScreens(ALRPlayerController* playerController);
	/**
	 * @brief 根据当前领域状态构建 Create Screen 所需的数据，不把临时对象作为长期存档标识。
	 * @param playerController 参与本次操作的运行时对象 `playerController`；函数会检查空值和所需接口。
	 * @param screen 本次操作使用的 `screen` 枚举或模式值。
	 * @param screenClass 调用方提供的 `screenClass`，只在本次操作范围内使用。
	 */
	void CreateScreen(ALRPlayerController* playerController, ELRScreenType screen, TSubclassOf<ULRScreenWidget> screenClass);
	/**
	 * @brief 更新 Screen Visible，并在需要时同步组件状态或广播变化事件。
	 * @param screen 本次操作使用的 `screen` 枚举或模式值。
	 * @param bVisible 布尔开关 `bVisible`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 */
	void SetScreenVisible(ELRScreenType screen, bool bVisible);
	/**
	 * @brief 隐藏所有互斥菜单页面，确保同一时刻只有一个焦点目标。
	 */
	void HideMenuScreens();

	/**
	 * @brief 尽力把菜单控制器绑定到角色库存；Possess 未发生时 Inventory 为空，SetObservedCharacter 会兜底完成绑定。
	 * @param playerController 参与本次操作的运行时对象 `playerController`；函数会检查空值和所需接口。
	 */
	void BindMenuControllerToCharacter(ALRPlayerController* playerController);

	/**
	 * @brief 查询当前内容集；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	class ULRGameContentSet* GetContentSet() const;

	/**
	 * @brief 查询当前 UI 调优；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	class ULRUITuning* GetUITuning() const;

	/**
	 * @brief 处理统一菜单 Tab 切换：显示单个菜单 Widget 并触发 OnMenuTabChanged。
	 * @param tab 本次操作使用的 `tab` 枚举或模式值。
	 */
	void ShowMenuTab(ELRScreenType tab);

	/**
	 * @brief 处理 Handle Narrative Presentation Changed 事件，将引擎回调转换为对应领域状态更新。
	 * @param presentation 本次领域操作的结构化数据 `presentation`；字段语义由对应 USTRUCT 定义。
	 */
	UFUNCTION()
	void HandleNarrativePresentationChanged(FLRNarrativePresentation presentation);

	/**
	 * @brief 处理 Handle Menu Screen Changed 事件，将引擎回调转换为对应领域状态更新。
	 * @param previousScreen 本次操作使用的 `previousScreen` 枚举或模式值。
	 * @param currentScreen 本次操作使用的 `currentScreen` 枚举或模式值。
	 */
	UFUNCTION()
	void HandleMenuScreenChanged(ELRScreenType previousScreen, ELRScreenType currentScreen);

	/**
	 * @brief 处理 Handle Transition Visibility Changed 事件，将引擎回调转换为对应领域状态更新。
	 * @param bVisible 布尔开关 `bVisible`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 */
	UFUNCTION()
	void HandleTransitionVisibilityChanged(bool bVisible);

	/** Screen Widgets 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TMap<ELRScreenType, TObjectPtr<ULRScreenWidget>> ScreenWidgets;

	/** Dialogue Controller 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TObjectPtr<ULRDialogueWidgetController> DialogueController;

	/** HUDController 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TObjectPtr<ULRHUDWidgetController> HUDController;

	/** Menu Controller 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TObjectPtr<ULRMenuWidgetController> MenuController;

	/** Transition Controller 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TObjectPtr<ULRTransitionWidgetController> TransitionController;

	UPROPERTY(Transient)
	TObjectPtr<ULRSaveWidgetController> SaveController;
};
