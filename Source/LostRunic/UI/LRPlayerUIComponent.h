/**
 * @file LRPlayerUIComponent.h
 * @brief 实现 HUD、状态遮罩、对话/阅读、统一菜单（背包/笔记/收藏）、暂停、存档槽和过场的控制器边界。UI 订阅领域事件并负责表现，不参与核心规则判定。
 *
 * 关联文件：LRPlayerUIComponent.cpp；所属领域：UI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Components/ActorComponent.h"
#include "Items/LRItemUseTypes.h"
#include "Narrative/LRNarrativeTypes.h"
#include "UI/LRUITypes.h"

#include "LRPlayerUIComponent.generated.h"

class ALRCharacter;
class ALRPlayerController;
class ULRScreenWidget;
class AActor;

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(ClassGroup = "Lost Runic", BlueprintType, meta = (BlueprintSpawnableComponent, DisplayName = "Lost Runic Player UI"))
class LOSTRUNIC_API ULRPlayerUIComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 */
	ULRPlayerUIComponent();

	/**
	 * @brief 解除委托并清理计时器或缓存，避免关卡切换和对象销毁后继续收到回调。
	 * @param endPlayReason Unreal 提供的结束原因，用于区分销毁、关卡切换和退出。
	 */
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

	/**
	 * @brief 创建并绑定本地玩家 UI；领域状态继续由角色组件和子系统拥有。
	 * @param playerController 参与本次操作的运行时对象 `playerController`；函数会检查空值和所需接口。
	 */
	void InitializeUI(ALRPlayerController* playerController);
	/**
	 * @brief 更新 Observed Character，并在需要时同步组件状态或广播变化事件。
	 * @param character 参与本次操作的运行时对象 `character`；函数会检查空值和所需接口。
	 */
	void SetObservedCharacter(ALRCharacter* character);

	/**
	 * @brief 处理 Handle Confirm 事件，将引擎回调转换为对应领域状态更新。
	 */
	void HandleConfirm();
	/**
	 * @brief 处理 Handle Cancel 事件，将引擎回调转换为对应领域状态更新。
	 */
	void HandleCancel();
	/**
	 * @brief 处理 Handle Open Inventory 事件：仅在 Gameplay 模式打开统一菜单背包页；菜单已打开时不处理（不关闭菜单）。
	 */
	void HandleOpenInventory();
	/**
	 * @brief 处理 Handle Pause 事件，将引擎回调转换为对应领域状态更新。
	 */
	void HandlePause();
	/**
	 * @brief 处理 Handle Navigate 事件：方向导航交给当前可聚焦 Screen（Slate Navigation 元数据）。
	 * @param direction 本次输入、状态更新或测试使用的值；二维输入只取绝对值较大的轴。
	 */
	void HandleNavigate(const FVector2D& direction);
	/**
	 * @brief 处理 Handle Previous Tab 事件：切换统一菜单上一页。
	 */
	void HandlePreviousTab();
	/**
	 * @brief 处理 Handle Next Tab 事件：切换统一菜单下一页。
	 */
	void HandleNextTab();
	/**
	 * @brief 处理 Handle UI Primary Action 事件：对当前 UI 条目执行主要业务动作（本界面为装备焦点武器）。
	 */
	void HandleUIPrimaryAction();
	/** 处理 UI Delete 语义；具体是否可删除由当前 Screen 与存档控制器判定。 */
	void HandleUIDelete();

	/**
	 * @brief 打开指定背包、笔记、收藏、暂停或存档页面，并切换到 Menu 输入上下文。
	 * @param screen 本次操作使用的 `screen` 枚举或模式值。
	 */
	void OpenMenuScreen(ELRScreenType screen);
	/**
	 * @brief 为需要物品的交互目标打开统一菜单的背包 Tab（交互选物模式）；只有与目标兼容的物品可提交。
	 * @param target 当前已通过交互筛选的物品使用目标。
	 */
	void OpenItemSelector(AActor* target);
	/**
	 * @brief 关闭当前菜单层并恢复仍然有效的下层输入上下文，同时抑制切换时仍按住的按键。
	 */
	void CloseMenuScreen();
	/**
	 * @brief 执行 Use Inventory Item 的玩法动作；输入层只提供语义，合法性由统一物品事务决定。
	 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	FLRItemUseResult UseInventoryItem(FName itemId) const;

	/**
	 * @brief 启用或关闭 Transition 输入层；关闭后恢复仍然有效的下层（Dialogue > Menu > Gameplay）。
	 * @param bActive 布尔开关 `bActive`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 */
	void SetTransitionLayer(bool bActive);
	/**
	 * @brief 启用或关闭 Dialogue 输入层；关闭后恢复仍然有效的下层（Menu > Gameplay）。
	 * @param bActive 布尔开关 `bActive`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 */
	void SetDialogueLayer(bool bActive);
	/**
	 * @brief 启用或关闭 Menu 输入层；关闭后恢复仍然有效的下层（Gameplay）。
	 * @param bActive 布尔开关 `bActive`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 */
	void SetMenuLayer(bool bActive);

	/**
	 * @brief 按 Transition > Dialogue > Menu > Gameplay 计算唯一有效输入层。
	 * @param bTransitionActive 布尔开关 `bTransitionActive`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 * @param bDialogueActive 布尔开关 `bDialogueActive`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 * @param bMenuActive 布尔开关 `bMenuActive`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	static ELRInputMode ComputeEffectiveInputMode(bool bTransitionActive, bool bDialogueActive, bool bMenuActive);

	/**
	 * @brief 查询当前仲裁结果；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|UI")
	ELRInputMode GetComputedInputMode() const;

private:
	/**
	 * @brief 处理 Handle Narrative Page Changed 事件，将引擎回调转换为对应领域状态更新。
	 * @param page 本次领域操作的结构化数据 `page`；字段语义由对应 USTRUCT 定义。
	 */
	UFUNCTION()
	void HandleNarrativePageChanged(FLRNarrativePage page);

	/**
	 * @brief 处理 Handle Narrative Session Ended 事件，将引擎回调转换为对应领域状态更新。
	 * @param sessionType 本次操作使用的 `sessionType` 枚举或模式值。
	 * @param finalContentId 稳定标识 `finalContentId`；用于内容查询和存档，不依赖显示名或数组序号。
	 */
	UFUNCTION()
	void HandleNarrativeSessionEnded(ELRNarrativeSessionType sessionType, FName finalContentId);

	/**
	 * @brief 把内部失败原因标签映射为面向玩家的友好提示，不暴露内部 Tag。
	 * @param failureReason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	FText DescribeItemUseFailure(FGameplayTag failureReason) const;
	/**
	 * @brief 查询 LRHUD；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	class ALRHUD* GetLRHUD() const;
	/**
	 * @brief 查询当前输入层对应的可聚焦 Screen；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	ULRScreenWidget* GetFocusableScreen() const;
	/**
	 * @brief 解除 UI 对叙事子系统的委托绑定，避免销毁或换图后重复回调。
	 */
	void UnbindNarrative();
	/**
	 * @brief 把仲裁出的唯一输入层应用到 Controller；只在值变化时调用 SetLRInputMode。
	 */
	void ApplyArbitratedInputMode();
	void SetWorldPaused(bool bPaused);

	/** Owner Controller 的内部运行时数据；不参与蓝图配置。 */
	TWeakObjectPtr<ALRPlayerController> OwnerController;
	/** Dialogue Subsystem 的内部运行时数据；不参与蓝图配置。 */
	TWeakObjectPtr<class ULRDialogueSubsystem> DialogueSubsystem;
	/** 交互选物模式的物品使用目标；为空时菜单为普通浏览模式。 */
	TWeakObjectPtr<AActor> ItemSelectorTarget;
	/** Transition 输入层激活状态；由 Save 子系统请求。 */
	bool bTransitionActive = false;
	/** Dialogue 输入层激活状态；由叙事会话驱动。 */
	bool bDialogueActive = false;
	/** Menu 输入层激活状态；由菜单开关驱动。 */
	bool bMenuActive = false;
	/** True while Pause or its Save page owns the real world pause. */
	bool bWorldPaused = false;
};
