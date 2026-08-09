/**
 * @file LRPlayerController.h
 * @brief 绑定 Enhanced Input 语义，把眼部、移动、交互、快捷栏、对话、菜单和过场输入路由到对应组件，并在上下文切换时抑制仍按住的按键。
 *
 * 关联文件：LRPlayerController.cpp；所属领域：Framework。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Core/LRTypes.h"
#include "GameFramework/PlayerController.h"
#include "Items/LRItemUseTypes.h"
#include "UI/LRUITypes.h"

#include "LRPlayerController.generated.h"

class UInputMappingContext;
class ULRInputConfig;
class ULRPlayerUIComponent;
struct FInputActionValue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLRInputModeChanged, ELRInputMode, previousMode, ELRInputMode, currentMode);

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Player Controller"))
class LOSTRUNIC_API ALRPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 */
	ALRPlayerController();

	/**
	 * @brief 在进入世界后解析运行时依赖、绑定事件并启动所需计时器；构造阶段不访问 World 或玩家对象。
	 */
	virtual void BeginPlay() override;
	/**
	 * @brief 绑定 PlayerController 使用的 Enhanced Input Action；具体按键仍由 Input Mapping Context 资产决定。
	 */
	virtual void SetupInputComponent() override;
	/**
	 * @brief 处理 On Possess 事件，将引擎回调转换为对应领域状态更新。
	 * @param pawn 参与本次操作的运行时对象 `pawn`；函数会检查空值和所需接口。
	 */
	virtual void OnPossess(APawn* pawn) override;

	/**
	 * @brief 更新 LRInput Mode，并在需要时同步组件状态或广播变化事件。
	 * @param newMode 本次操作使用的 `newMode` 枚举或模式值。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Input")
	void SetLRInputMode(ELRInputMode newMode);

	/**
	 * @brief 查询 LRInput Mode；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Input")
	ELRInputMode GetLRInputMode() const { return InputMode; }

	/**
	 * @brief 打开指定背包、笔记、收藏、暂停或存档页面，并切换到 Menu 输入上下文。
	 * @param screen 本次操作使用的 `screen` 枚举或模式值。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|UI")
	void OpenMenuScreen(ELRScreenType screen);

	/**
	 * @brief 关闭当前菜单层并恢复 Gameplay 输入上下文，同时抑制切换时仍按住的按键。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|UI")
	void CloseMenuScreen();

	/**
	 * @brief 执行 Use Inventory Item From Menu 的玩法动作；输入层只提供语义，合法性由对应领域组件决定。
	 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Inventory")
	FLRItemUseResult UseInventoryItemFromMenu(FName itemId);

	/** 当 Input Mode Changed 发生时广播；蓝图可绑定该委托以更新表现，不应在回调中改写核心规则。  */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Input")
	FLRInputModeChanged OnInputModeChanged;

protected:
	/** Input Config 的领域数据，由所属类型负责维护和校验。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<ULRInputConfig> InputConfig;

private:
	/**
	 * @brief 处理 Handle Move 事件，将引擎回调转换为对应领域状态更新。
	 * @param value 本次输入、状态更新或测试使用的值。
	 */
	void HandleMove(const FInputActionValue& value);
	/**
	 * @brief 处理 Handle Sneak Toggle 事件，将引擎回调转换为对应领域状态更新。
	 */
	void HandleSneakToggle();
	/**
	 * @brief 处理 Handle Run Started 事件，将引擎回调转换为对应领域状态更新。
	 */
	void HandleRunStarted();
	/**
	 * @brief 处理 Handle Run Stopped 事件，将引擎回调转换为对应领域状态更新。
	 */
	void HandleRunStopped();
	/**
	 * @brief 处理 Handle Close Eyes Started 事件，将引擎回调转换为对应领域状态更新。
	 */
	void HandleCloseEyesStarted();
	/**
	 * @brief 处理 Handle Close Eyes Stopped 事件，将引擎回调转换为对应领域状态更新。
	 */
	void HandleCloseEyesStopped();
	/**
	 * @brief 处理 Handle Open Eyes Started 事件，将引擎回调转换为对应领域状态更新。
	 */
	void HandleOpenEyesStarted();
	/**
	 * @brief 处理 Handle Open Eyes Stopped 事件，将引擎回调转换为对应领域状态更新。
	 */
	void HandleOpenEyesStopped();
	/**
	 * @brief 处理 Handle Interact 事件，将引擎回调转换为对应领域状态更新。
	 */
	void HandleInteract();
	/**
	 * @brief 处理 Handle Quick Slot1 事件，将引擎回调转换为对应领域状态更新。
	 */
	void HandleQuickSlot1();
	/**
	 * @brief 处理 Handle Quick Slot2 事件，将引擎回调转换为对应领域状态更新。
	 */
	void HandleQuickSlot2();
	/**
	 * @brief 处理 Handle Quick Slot3 事件，将引擎回调转换为对应领域状态更新。
	 */
	void HandleQuickSlot3();
	/**
	 * @brief 处理 Handle Quick Slot4 事件，将引擎回调转换为对应领域状态更新。
	 */
	void HandleQuickSlot4();
	/**
	 * @brief 处理 Handle Use Selected Quick Slot 事件，将引擎回调转换为对应领域状态更新。
	 */
	void HandleUseSelectedQuickSlot();
	/**
	 * @brief 处理 Handle Previous Quick Slot 事件，将引擎回调转换为对应领域状态更新。
	 */
	void HandlePreviousQuickSlot();
	/**
	 * @brief 处理 Handle Next Quick Slot 事件，将引擎回调转换为对应领域状态更新。
	 */
	void HandleNextQuickSlot();
	/**
	 * @brief 处理 Handle Confirm 事件，将引擎回调转换为对应领域状态更新。
	 */
	void HandleConfirm();
	/**
	 * @brief 处理 Handle Cancel 事件，将引擎回调转换为对应领域状态更新。
	 */
	void HandleCancel();
	/**
	 * @brief 处理 Handle Open Journal 事件，将引擎回调转换为对应领域状态更新。
	 */
	void HandleOpenJournal();
	/**
	 * @brief 处理 Handle Pause 事件，将引擎回调转换为对应领域状态更新。
	 */
	void HandlePause();
	/**
	 * @brief 从指定快捷栏取得物品并通过统一物品事务作用于当前目标。
	 * @param slotIndex 槽位下标；快捷栏为 0-3，手动存档槽按调优上限校验。
	 */
	void UseQuickSlot(int32 slotIndex);
	/**
	 * @brief 执行 Resolve Context 的纯规则或事务判定，失败时提供结构化原因。
	 * @param mode 本次操作使用的 `mode` 枚举或模式值。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UInputMappingContext* ResolveContext(ELRInputMode mode) const;
	/**
	 * @brief 根据最新领域状态刷新 Update State Input Blocker，并仅在值变化时通知订阅者。
	 * @param previousMode 本次操作使用的 `previousMode` 枚举或模式值。
	 * @param newMode 本次操作使用的 `newMode` 枚举或模式值。
	 */
	void UpdateStateInputBlocker(ELRInputMode previousMode, ELRInputMode newMode);
	/**
	 * @brief 根据 Gameplay、Dialogue、Menu、Transition 模式设置鼠标、焦点和输入捕获。
	 * @param newMode 本次操作使用的 `newMode` 枚举或模式值。
	 */
	void ConfigureViewportInput(ELRInputMode newMode);
	/**
	 * @brief 查询 LRHUD；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	class ALRHUD* GetLRHUD() const;

	/** Player UI 的领域数据，由所属类型负责维护和校验。 仅在蓝图或详情面板中查看，不可编辑。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULRPlayerUIComponent> PlayerUI;

	/** Input Mode 的内部运行时数据；不参与蓝图配置。 */
	ELRInputMode InputMode = ELRInputMode::Gameplay;
};
