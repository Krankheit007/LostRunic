/**
 * @file LRScreenWidget.h
 * @brief 实现 HUD、状态遮罩、对话/阅读、背包/笔记/收藏、暂停、存档槽和过场的控制器边界。UI 订阅领域事件并负责表现，不参与核心规则判定。
 *
 * 关联文件：LRScreenWidget.cpp；所属领域：UI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Blueprint/UserWidget.h"
#include "UI/LRUITypes.h"

#include "LRScreenWidget.generated.h"

class ULRHUDWidgetController;
class ULRSaveWidgetController;

/**
 * @brief 由 Widget Blueprint 的 Navigation 元数据驱动的通用 UI 输入宿主。
 *
 * 本类只理解通用命令（ELRUICommand）、方向导航（HandleNavigate）与焦点生命周期（SetInitialFocus/RestoreFocus），
 * 不理解背包、笔记、收藏品或槽位数量；具体页面由子类（如 ULRInventoryScreenWidget）解释命令并保存焦点索引。
 * 方向导航以 Slate 为唯一系统：将方向转换为单一 EUINavigation 后调用 FSlateApplication::NavigateFromWidget()，
 * Designer 配置的 Stop/Wrap/Explicit/Custom/CustomBoundary 具有绝对权威；fallback 只负责恢复“至少存在一个合法焦点”。
 */
UCLASS(Abstract, Blueprintable, meta = (DisplayName = "Lost Runic Screen Widget"))
class LOSTRUNIC_API ULRScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * @brief 更新 Screen Visible，并在需要时同步组件状态或广播变化事件。
	 * @param bVisible 布尔开关 `bVisible`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 */
	virtual void SetScreenVisible(bool bVisible);
	/**
	 * @brief 把当前叙事页面数据推送到 Widget 表现，不执行剧情条件或存档规则。
	 * @param presentation 本次领域操作的结构化数据 `presentation`；字段语义由对应 USTRUCT 定义。
	 */
	void PresentNarrative(const FLRNarrativePresentation& presentation);

	/**
	 * @brief 查询 Screen Type；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	ELRScreenType GetScreenType() const { return ScreenType; }
	/**
	 * @brief 判断 Is Screen Visible 对应条件；不产生玩法副作用。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool IsScreenVisible() const { return bScreenVisible; }

	/**
	 * @brief 处理通用 UI 命令（Confirm/Cancel/PreviousTab/NextTab/PrimaryAction）；基类默认不处理任何命令。
	 * @param command 本次操作使用的 `command` 枚举或模式值。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Lost Runic|UI")
	bool HandleUICommand(ELRUICommand command);
	virtual bool HandleUICommand_Implementation(ELRUICommand command);

	/**
	 * @brief 按方向处理导航：把方向转换为单一 EUINavigation 后交给 Slate（Widget Blueprint Navigation 元数据），
	 *        Slate 成功移动焦点才返回成功；焦点无效时才执行故障恢复（RestoreFocus -> SetInitialFocus -> 自身焦点）。
	 * @param direction 本次输入、状态更新或测试使用的值；二维输入只取绝对值较大的轴。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	virtual bool HandleNavigate(const FVector2D& direction);

	/**
	 * @brief 设置初始焦点：恢复当前 Tab 保存的有效索引，无效时落到第一个可用条目；基类默认聚焦 Screen 自身。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	virtual bool SetInitialFocus();
	/**
	 * @brief 恢复会话内保存的焦点索引；基类默认等同 SetInitialFocus。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	virtual bool RestoreFocus();

	/**
	 * @brief 处理 On Screen Visibility Changed 事件，将引擎回调转换为对应领域状态更新。
	 * @param bVisible 布尔开关 `bVisible`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Lost Runic|UI")
	void OnScreenVisibilityChanged(bool bVisible);

	/**
	 * @brief 处理 On Narrative Presentation Changed 事件，将引擎回调转换为对应领域状态更新。
	 * @param presentation 本次领域操作的结构化数据 `presentation`；字段语义由对应 USTRUCT 定义。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Lost Runic|UI")
	void OnNarrativePresentationChanged(const FLRNarrativePresentation& presentation);

	/**
	 * @brief 菜单显示友好状态消息（如“物品已满！”）；内部失败原因标签已由 C++ 映射，不直接暴露给玩家。
	 * @param message 调用方提供的 `message`，只在本次操作范围内使用。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Lost Runic|UI")
	void OnMenuStatusMessage(const FText& message);

	/** Called after ALRHUD injects the controller that owns HUD state and interaction events. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Lost Runic|UI")
	void OnHUDWidgetControllerReady(ULRHUDWidgetController* controller);

	UFUNCTION(BlueprintImplementableEvent, Category = "Lost Runic|Save UI")
	void OnSaveWidgetControllerReady(ULRSaveWidgetController* controller);

	/** Returns the controller injected by ALRHUD for this local player's screen. */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|UI")
	ULRHUDWidgetController* GetHUDWidgetController() const { return HUDWidgetController; }

	/** Injects the local HUD event source before the widget is exposed to gameplay. */
	virtual void SetHUDWidgetController(ULRHUDWidgetController* controller);
	virtual void SetSaveWidgetController(ULRSaveWidgetController* controller);

protected:
	/**
	 * @brief 在 UMG 原生初始化阶段建立 Widget 自身状态；领域事件由外部控制器绑定。
	 */
	virtual void NativeOnInitialized() override;

	/**
	 * @brief 把焦点移动到指定 Widget；目标必须可见、启用且支持键盘焦点。
	 * @param widget 参与本次操作的运行时对象 `widget`；函数会检查空值和所需接口。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool SetFocusToWidget(UWidget* widget) const;

	/**
	 * @brief 查询当前 Slate User Index；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	int32 GetSlateUserIndex() const;

	/** Screen Type 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `ELRScreenType::None`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Screen")
	ELRScreenType ScreenType = ELRScreenType::None;

	/** Runtime controller reference; the HUD owns the controller and the widget never owns gameplay state. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Screen", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULRHUDWidgetController> HUDWidgetController;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Screen", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULRSaveWidgetController> SaveWidgetController;

private:
	/**
	 * @brief 判断当前 User Focus 是否仍属于本 Screen 且可见、启用、可聚焦；Screen 自身焦点也视为有效。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool IsCurrentFocusValid();

	/** Screen Visible 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	bool bScreenVisible = false;
};
