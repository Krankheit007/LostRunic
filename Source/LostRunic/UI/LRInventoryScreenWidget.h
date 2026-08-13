/**
 * @file LRInventoryScreenWidget.h
 * @brief 统一菜单（背包 4×2 / 笔记 1×12 / 收藏品 4×3）的具体 Screen：解释通用命令、维护 Tab、选中项、装备武器与各 Tab 焦点索引恢复；只消费 FLRInventorySnapshot 表现数据，动作一律回到 ULRMenuWidgetController / ULRInventoryComponent。
 *
 * 关联文件：LRInventoryScreenWidget.cpp；所属领域：UI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "UI/LRScreenWidget.h"

#include "LRInventoryScreenWidget.generated.h"

class UButton;
class UImage;
class UScrollBox;
class UTextBlock;
class UTexture2D;
class UWidgetSwitcher;
class ULRMenuWidgetController;

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Inventory Screen Widget"))
class LOSTRUNIC_API ULRInventoryScreenWidget : public ULRScreenWidget
{
	GENERATED_BODY()

public:
	/**
	 * @brief 在 UMG 原生初始化阶段建立 Widget 自身状态；领域事件由外部控制器绑定。
	 */
	virtual void NativeOnInitialized() override;
	/**
	 * @brief 解除委托并清理计时器或缓存，避免关卡切换和对象销毁后继续收到回调。
	 */
	virtual void NativeDestruct() override;
	/**
	 * @brief 关闭菜单时清空各 Tab 焦点索引、选择 ID 与详情缓存，并取消待执行的焦点请求；显示时恢复。
	 * @param bVisible 布尔开关 `bVisible`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 */
	virtual void SetScreenVisible(bool bVisible) override;

	/**
	 * @brief 切换统一菜单的当前 Tab（背包/笔记/收藏品）并恢复该 Tab 的会话焦点索引。
	 * @param tab 本次操作使用的 `tab` 枚举或模式值；非菜单页值回退到背包页。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|UI")
	void SetActiveTab(ELRScreenType tab);

	/**
	 * @brief 确认选中背包第 index 格（索引 0 起）：更新当前选择与详情，不装备武器。
	 * @param index 本次操作使用的计数、增量或索引 `index`；由函数校验合法范围。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|UI")
	void SelectBagItem(int32 index);
	/**
	 * @brief 确认选中第 index 条笔记（索引 0 起）：更新当前选择与详情。
	 * @param index 本次操作使用的计数、增量或索引 `index`；由函数校验合法范围。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|UI")
	void SelectNote(int32 index);
	/**
	 * @brief 确认选中第 index 件收藏品（索引 0 起）：更新当前选择与详情。
	 * @param index 本次操作使用的计数、增量或索引 `index`；由函数校验合法范围。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|UI")
	void SelectCollectible(int32 index);

	/**
	 * @brief 装备当前已确认选择的武器（Choose_Weapon_BTN 点击入口）；合法性由 InventoryComponent 校验。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|UI")
	void EquipSelectedWeapon();
	/**
	 * @brief 装备当前焦点所在的背包武器（PrimaryAction 入口）；焦点不在武器格时不装备。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|UI")
	bool EquipWeaponAtFocus();

	/** 返回缓存详情文本；蓝图文本绑定函数只调用这些 getter，不得查询 Inventory、DataTable 或 DataAsset。 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|UI")
	FText GetCachedBagName() const { return CachedBagName; }
	UFUNCTION(BlueprintPure, Category = "Lost Runic|UI")
	FText GetCachedBagInfo() const { return CachedBagInfo; }
	UFUNCTION(BlueprintPure, Category = "Lost Runic|UI")
	FText GetCachedNoteName() const { return CachedNoteName; }
	UFUNCTION(BlueprintPure, Category = "Lost Runic|UI")
	FText GetCachedNoteInfo() const { return CachedNoteInfo; }
	UFUNCTION(BlueprintPure, Category = "Lost Runic|UI")
	FText GetCachedCollectibleName() const { return CachedCollectibleName; }
	UFUNCTION(BlueprintPure, Category = "Lost Runic|UI")
	FText GetCachedCollectibleInfo() const { return CachedCollectibleInfo; }

	/** 查询 Current Tab；不修改领域状态。 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|UI")
	ELRScreenType GetCurrentTab() const { return CurrentTab; }

	/**
	 * @brief 注入统一菜单控制器（由 ALRHUD 在创建 Screen 时调用），并绑定快照变化事件。
	 * @param controller 参与本次操作的运行时对象 `controller`；函数会检查空值和所需接口。
	 */
	void SetMenuWidgetController(ULRMenuWidgetController* controller);

	/**
	 * @brief 应用新快照并刷新全部 Tab；无效快照让 UI fail closed（全部槽位禁用、详情清空、装备按钮隐藏）。
	 * @param snapshot 本次领域操作的结构化数据 `snapshot`；字段语义由对应 USTRUCT 定义。
	 */
	void SetSnapshot(const FLRInventorySnapshot& snapshot);

protected:
	/**
	 * @brief 解释通用命令：Confirm 确认当前焦点条目、Cancel 关闭菜单、PreviousTab/NextTab 切换页面、PrimaryAction 装备焦点武器。
	 * @param command 本次操作使用的 `command` 枚举或模式值。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	virtual bool HandleUICommand_Implementation(ELRUICommand command) override;
	/**
	 * @brief 笔记页的 Note_Roll 聚焦时 Up/Down 按调优步长滚动、Left 返回 LastNoteIndex；其余交给基类 Slate 导航。
	 * @param direction 本次输入、状态更新或测试使用的值；二维输入只取绝对值较大的轴。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	virtual bool HandleNavigate(const FVector2D& direction) override;
	/**
	 * @brief 恢复当前 Tab 保存的焦点索引；索引无效或条目不可聚焦时失败，由基类回退到 SetInitialFocus。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	virtual bool RestoreFocus() override;
	/**
	 * @brief 设置当前 Tab 的初始焦点：第一个可用（可见、启用、可聚焦）条目；全空时失败，由基类聚焦 Screen 自身。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	virtual bool SetInitialFocus() override;

private:
	/** 固定槽位点击处理器；在 NativeOnInitialized 中绑定 UButton::OnClicked，蓝图无需逐槽接线。 */
	UFUNCTION() void HandleBagItem1Clicked();	UFUNCTION() void HandleBagItem2Clicked();
	UFUNCTION() void HandleBagItem3Clicked();	UFUNCTION() void HandleBagItem4Clicked();
	UFUNCTION() void HandleBagItem5Clicked();	UFUNCTION() void HandleBagItem6Clicked();
	UFUNCTION() void HandleBagItem7Clicked();	UFUNCTION() void HandleBagItem8Clicked();
	UFUNCTION() void HandleNote1Clicked();		UFUNCTION() void HandleNote2Clicked();
	UFUNCTION() void HandleNote3Clicked();		UFUNCTION() void HandleNote4Clicked();
	UFUNCTION() void HandleNote5Clicked();		UFUNCTION() void HandleNote6Clicked();
	UFUNCTION() void HandleNote7Clicked();		UFUNCTION() void HandleNote8Clicked();
	UFUNCTION() void HandleNote9Clicked();		UFUNCTION() void HandleNote10Clicked();
	UFUNCTION() void HandleNote11Clicked();		UFUNCTION() void HandleNote12Clicked();
	UFUNCTION() void HandleCollectible1Clicked();	UFUNCTION() void HandleCollectible2Clicked();
	UFUNCTION() void HandleCollectible3Clicked();	UFUNCTION() void HandleCollectible4Clicked();
	UFUNCTION() void HandleCollectible5Clicked();	UFUNCTION() void HandleCollectible6Clicked();
	UFUNCTION() void HandleCollectible7Clicked();	UFUNCTION() void HandleCollectible8Clicked();
	UFUNCTION() void HandleCollectible9Clicked();	UFUNCTION() void HandleCollectible10Clicked();
	UFUNCTION() void HandleCollectible11Clicked();	UFUNCTION() void HandleCollectible12Clicked();
	UFUNCTION() void HandleChooseWeaponClicked();

	/**
	 * @brief 处理快照变化事件：刷新全部 Tab 表现，清空已消失的选择并重新验证焦点索引。
	 * @param snapshot 本次领域操作的结构化数据 `snapshot`；字段语义由对应 USTRUCT 定义。
	 */
	UFUNCTION()
	void HandleSnapshotChanged(const FLRInventorySnapshot& snapshot);

	/**
	 * @brief 对所有可聚焦控件设置 Slate Next/Previous 导航为 Stop：Tab 键只由 NextTabAction 消费切页，不触发默认焦点遍历。
	 */
	void ApplyTabNavigationGuard();
	/**
	 * @brief 按快照刷新背包页：空槽清空 Brush 并 Disabled，有物品时启用并设置图标，武器标识只显示 SelectedWeaponItemId 对应槽位。
	 */
	void RefreshBag();
	/**
	 * @brief 按快照刷新笔记页：Locked 显示“？？？”并 Disabled，Unlocked 显示标题并可选择。
	 */
	void RefreshNotes();
	/**
	 * @brief 按快照刷新收藏品页：Locked 使用剪影 Brush 并 Disabled，Unlocked 使用真实 Icon 并启用。
	 */
	void RefreshCollectibles();
	/**
	 * @brief 选择已消失时清空选择、详情缓存并隐藏装备按钮；刷新后重新验证当前 Tab 的焦点索引。
	 */
	void HandleSelectionOutOfDate();
	/**
	 * @brief 把三个详情区的缓存文本写入 TextBlock；蓝图绑定函数方案因 UMG 绑定数据不可脚本化而由 C++ 直连替代。
	 */
	void RefreshDetailsText();
	/**
	 * @brief 根据当前选择与 Tab 显示/启用 Choose_Weapon_BTN；非武器、非背包 Tab 或关闭菜单时 Collapsed 且 Disabled。
	 */
	void UpdateEquipButton();
	/**
	 * @brief 设置按钮 Normal/Hovered/Pressed Brush；icon 为空时清空为 NoDrawType。
	 * @param button 参与本次操作的运行时对象 `button`；函数会检查空值和所需接口。
	 * @param icon 参与本次操作的运行时对象 `icon`；函数会检查空值和所需接口。
	 */
	void SetButtonIcon(UButton* button, UTexture2D* icon) const;
	/**
	 * @brief 查询当前用户焦点对应的背包槽索引（0 起）；不在背包槽位上时返回 -1。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	int32 GetFocusedBagIndex() const;
	/**
	 * @brief 查询当前用户焦点对应的笔记槽索引（0 起）；不在笔记槽位上时返回 -1。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	int32 GetFocusedNoteIndex() const;
	/**
	 * @brief 查询当前用户焦点对应的收藏品槽索引（0 起）；不在收藏品槽位上时返回 -1。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	int32 GetFocusedCollectibleIndex() const;
	/**
	 * @brief 判断当前用户焦点是否落在指定按钮上。
	 * @param button 参与本次操作的运行时对象 `button`；函数会检查空值和所需接口。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool IsButtonFocused(const UButton* button) const;
	/**
	 * @brief 记录当前焦点所在的固定槽位索引；焦点不在任何槽位时保持原值。
	 */
	void TrackFocusedIndex();
	/**
	 * @brief 把焦点移动到指定固定按钮并记录该 Tab 的会话焦点索引；目标必须可用。
	 * @param index 本次操作使用的计数、增量或索引 `index`；由函数校验合法范围。
	 * @param button 参与本次操作的运行时对象 `button`；函数会检查空值和所需接口。
	 * @param lastIndex 参与本次操作的运行时对象 `lastIndex`；函数会检查空值和所需接口。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool FocusSlot(int32 index, UButton* button, int32& lastIndex);
	/**
	 * @brief 判断当前用户焦点是否落在笔记列表 ScrollBox 上。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool IsNoteScrollBoxFocused() const;
	/**
	 * @brief 查询笔记列表滚动步长；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	float GetNoteScrollStep() const;
	/**
	 * @brief 在布局完成后的 next-tick 恢复当前 Tab 的会话焦点；关闭界面时通过 SetScreenVisible(false) 取消。
	 */
	void RequestFocusRestore();
	/**
	 * @brief 取消待执行的 next-tick 焦点请求。
	 */
	void CancelPendingFocus();
	/**
	 * @brief 执行待执行的焦点恢复回调。
	 */
	void HandlePendingFocusRestore();
	/**
	 * @brief 恢复背包页会话焦点索引；索引有效且条目可聚焦时聚焦并返回成功。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool RestoreBagFocus();
	/**
	 * @brief 恢复笔记页会话焦点索引；索引有效且条目可聚焦时聚焦并返回成功。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool RestoreNoteFocus();
	/**
	 * @brief 恢复收藏品页会话焦点索引；索引有效且条目可聚焦时聚焦并返回成功。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool RestoreCollectibleFocus();
	/**
	 * @brief 聚焦背包页第一个可用条目。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool FocusFirstEnabledBagItem();
	/**
	 * @brief 聚焦笔记页第一个可用条目。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool FocusFirstEnabledNote();
	/**
	 * @brief 聚焦收藏品页第一个可用条目。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool FocusFirstEnabledCollectible();
	/**
	 * @brief 把指定的固定槽位按钮切换为“已选中/已打开”状态：记录焦点索引并更新选择详情。
	 * @param index 本次操作使用的计数、增量或索引 `index`；由函数校验合法范围。
	 */
	void UpdateBagSelectionState(int32 index);
	/**
	 * @brief 关闭统一菜单；经 PlayerController 回到 PlayerUIComponent 的菜单层仲裁。
	 */
	void CloseMenu();

	/** 按索引返回固定背包按钮；越界返回空。 */
	UButton* GetBagButton(int32 index) const;
	/** 按索引返回固定笔记按钮；越界返回空。 */
	UButton* GetNoteButton(int32 index) const;
	/** 按索引返回固定收藏品按钮；越界返回空。 */
	UButton* GetCollectibleButton(int32 index) const;
	/** 按索引返回固定背包数量文本；越界返回空。 */
	UTextBlock* GetBagCountText(int32 index) const;
	/** 按索引返回固定笔记标题文本；越界返回空。 */
	UTextBlock* GetNoteTitleText(int32 index) const;
	/** 按索引返回固定武器标识 Image；越界返回空。 */
	UImage* GetBagWeaponMarker(int32 index) const;

	/** Content WidgetSwitcher，按 Tab 索引切换页面。 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UWidgetSwitcher> Content;

	/** 顶部 Tab 按钮；OnClicked 由蓝图接线到 SetActiveTab，C++ 只用于焦点守卫与可聚焦集合。 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UButton> Bag;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UButton> Note;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UButton> Col;

	/** 背包页固定槽位按钮 1~8。 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UButton> Bag_Item_BTN_1;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UButton> Bag_Item_BTN_2;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UButton> Bag_Item_BTN_3;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UButton> Bag_Item_BTN_4;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UButton> Bag_Item_BTN_5;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UButton> Bag_Item_BTN_6;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UButton> Bag_Item_BTN_7;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UButton> Bag_Item_BTN_8;

	/** 背包页固定槽位数量文本 1~8。 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UTextBlock> Bag_Item_Cnt_1;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UTextBlock> Bag_Item_Cnt_2;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UTextBlock> Bag_Item_Cnt_3;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UTextBlock> Bag_Item_Cnt_4;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UTextBlock> Bag_Item_Cnt_5;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UTextBlock> Bag_Item_Cnt_6;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UTextBlock> Bag_Item_Cnt_7;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UTextBlock> Bag_Item_Cnt_8;

	/** 背包页武器标识 Image 1~8；默认 Collapsed，仅 SelectedWeaponItemId 对应槽位显示。 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UImage> Bag_Weapon_1;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UImage> Bag_Weapon_2;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UImage> Bag_Weapon_3;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UImage> Bag_Weapon_4;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UImage> Bag_Weapon_5;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UImage> Bag_Weapon_6;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UImage> Bag_Weapon_7;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UImage> Bag_Weapon_8;

	/** 装备当前已确认武器的按钮；仅选择武器时显示并启用。 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UButton> Choose_Weapon_BTN;

	/** 笔记页固定条目按钮 1~12。 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UButton> Note_1_BTN;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UButton> Note_2_BTN;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UButton> Note_3_BTN;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UButton> Note_4_BTN;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UButton> Note_5_BTN;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UButton> Note_6_BTN;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UButton> Note_7_BTN;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UButton> Note_8_BTN;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UButton> Note_9_BTN;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UButton> Note_10_BTN;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UButton> Note_11_BTN;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UButton> Note_12_BTN;

	/** 笔记页固定标题文本 1~12；Locked 时为“？？？”。 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UTextBlock> Note_1_T;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UTextBlock> Note_2_T;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UTextBlock> Note_3_T;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UTextBlock> Note_4_T;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UTextBlock> Note_5_T;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UTextBlock> Note_6_T;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UTextBlock> Note_7_T;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UTextBlock> Note_8_T;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UTextBlock> Note_9_T;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UTextBlock> Note_10_T;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UTextBlock> Note_11_T;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UTextBlock> Note_12_T;

	/** 笔记列表 ScrollBox；可聚焦，聚焦时 Up/Down 滚动、Left 返回列表。 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UScrollBox> Note_Roll;

	/** 详情区 TextBlock；由 C++ 在快照刷新/选择变化时直接 SetText（蓝图绑定函数方案因 UMG 绑定数据不可脚本化而弃用，见 06_BlueprintConfigurationGuide.md）。 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UTextBlock> Bag_Item_Name;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UTextBlock> Bag_Item_Info;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UTextBlock> Note_Name_T;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UTextBlock> Note_Info_T;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UTextBlock> Col_Name_T;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UTextBlock> Col_Info_T;

	/** 收藏品页固定图标按钮 1~12。 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UButton> Col_Icon_1;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UButton> Col_Icon_2;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UButton> Col_Icon_3;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UButton> Col_Icon_4;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UButton> Col_Icon_5;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UButton> Col_Icon_6;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UButton> Col_Icon_7;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UButton> Col_Icon_8;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UButton> Col_Icon_9;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UButton> Col_Icon_10;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UButton> Col_Icon_11;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true")) TObjectPtr<UButton> Col_Icon_12;

	/** 当前快照（只读 View Model）；Widget 只消费，不修改。 */
	FLRInventorySnapshot Snapshot;

	/** 菜单控制器引用；由 HUD 注入，装备等动作经其回到领域。 */
	UPROPERTY(Transient)
	TObjectPtr<ULRMenuWidgetController> MenuController;

	/** Current Tab 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	ELRScreenType CurrentTab = ELRScreenType::Journal;

	/** 当前确认选择的稳定 ID；None 表示未选择。 */
	FName CurrentBagItemId = NAME_None;
	FName CurrentReadingId = NAME_None;
	FName CurrentCollectibleId = NAME_None;

	/** 各 Tab 会话焦点索引；关闭菜单清空，Tab 内切换保留。 */
	int32 LastBagIndex = INDEX_NONE;
	int32 LastNoteIndex = INDEX_NONE;
	int32 LastCollectibleIndex = INDEX_NONE;

	/** 详情缓存；蓝图文本绑定函数只读这些值。 */
	FText CachedBagName;
	FText CachedBagInfo;
	FText CachedNoteName;
	FText CachedNoteInfo;
	FText CachedCollectibleName;
	FText CachedCollectibleInfo;

	/** 待执行的 next-tick 焦点请求。 */
	FTimerHandle PendingFocusTimer;
};
