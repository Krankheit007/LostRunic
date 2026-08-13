/**
 * @file LRInventoryScreenWidget.cpp
 * @brief 统一菜单（背包 4×2 / 笔记 1×12 / 收藏品 4×3）的具体 Screen：解释通用命令、维护 Tab、选中项、装备武器与各 Tab 焦点索引恢复；只消费 FLRInventorySnapshot 表现数据，动作一律回到 ULRMenuWidgetController / ULRInventoryComponent。
 *
 * 关联文件：LRInventoryScreenWidget.h；所属领域：UI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "UI/LRInventoryScreenWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Core/LRLog.h"
#include "Data/LRGameTuningSet.h"
#include "Data/LRUITuning.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "Framework/LRPlayerController.h"
#include "Framework/Application/SlateApplication.h"
#include "Items/LRInventoryComponent.h"
#include "UI/LRMenuWidgetController.h"

namespace
{
	/** 背包页 WidgetSwitcher 子页索引。 */
	constexpr int32 BagTabIndex = 0;
	/** 笔记页 WidgetSwitcher 子页索引。 */
	constexpr int32 NoteTabIndex = 1;
	/** 收藏品页 WidgetSwitcher 子页索引。 */
	constexpr int32 CollectibleTabIndex = 2;

	/** 固定槽位容量；与 LRMenuCapacity 保持一致。 */
	constexpr int32 BagSlotCount = 8;
	constexpr int32 NoteSlotCount = 12;
	constexpr int32 CollectibleSlotCount = 12;

	/**
	 * @brief 把 Tab 页枚举转换为 WidgetSwitcher 子页索引。
	 * @param tab 本次操作使用的 `tab` 枚举或模式值。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	int32 TabToSwitcherIndex(const ELRScreenType tab)
	{
		if (tab == ELRScreenType::Inventory)
		{
			return BagTabIndex;
		}
		if (tab == ELRScreenType::Collectibles)
		{
			return CollectibleTabIndex;
		}
		return NoteTabIndex;
	}

	/**
	 * @brief 返回 Tab 的前一页（环绕）。
	 * @param tab 本次操作使用的 `tab` 枚举或模式值。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	ELRScreenType PreviousTabOf(const ELRScreenType tab)
	{
		if (tab == ELRScreenType::Inventory)
		{
			return ELRScreenType::Collectibles;
		}
		if (tab == ELRScreenType::Collectibles)
		{
			return ELRScreenType::Journal;
		}
		return ELRScreenType::Inventory;
	}

	/**
	 * @brief 返回 Tab 的下一页（环绕）。
	 * @param tab 本次操作使用的 `tab` 枚举或模式值。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	ELRScreenType NextTabOf(const ELRScreenType tab)
	{
		if (tab == ELRScreenType::Inventory)
		{
			return ELRScreenType::Journal;
		}
		if (tab == ELRScreenType::Collectibles)
		{
			return ELRScreenType::Inventory;
		}
		return ELRScreenType::Collectibles;
	}
}

/**
 * @brief 在 UMG 原生初始化阶段建立 Widget 自身状态；领域事件由外部控制器绑定。
 */
void ULRInventoryScreenWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	Bag_Item_BTN_1->OnClicked.AddDynamic(this, &ULRInventoryScreenWidget::HandleBagItem1Clicked);
	Bag_Item_BTN_2->OnClicked.AddDynamic(this, &ULRInventoryScreenWidget::HandleBagItem2Clicked);
	Bag_Item_BTN_3->OnClicked.AddDynamic(this, &ULRInventoryScreenWidget::HandleBagItem3Clicked);
	Bag_Item_BTN_4->OnClicked.AddDynamic(this, &ULRInventoryScreenWidget::HandleBagItem4Clicked);
	Bag_Item_BTN_5->OnClicked.AddDynamic(this, &ULRInventoryScreenWidget::HandleBagItem5Clicked);
	Bag_Item_BTN_6->OnClicked.AddDynamic(this, &ULRInventoryScreenWidget::HandleBagItem6Clicked);
	Bag_Item_BTN_7->OnClicked.AddDynamic(this, &ULRInventoryScreenWidget::HandleBagItem7Clicked);
	Bag_Item_BTN_8->OnClicked.AddDynamic(this, &ULRInventoryScreenWidget::HandleBagItem8Clicked);
	Note_1_BTN->OnClicked.AddDynamic(this, &ULRInventoryScreenWidget::HandleNote1Clicked);
	Note_2_BTN->OnClicked.AddDynamic(this, &ULRInventoryScreenWidget::HandleNote2Clicked);
	Note_3_BTN->OnClicked.AddDynamic(this, &ULRInventoryScreenWidget::HandleNote3Clicked);
	Note_4_BTN->OnClicked.AddDynamic(this, &ULRInventoryScreenWidget::HandleNote4Clicked);
	Note_5_BTN->OnClicked.AddDynamic(this, &ULRInventoryScreenWidget::HandleNote5Clicked);
	Note_6_BTN->OnClicked.AddDynamic(this, &ULRInventoryScreenWidget::HandleNote6Clicked);
	Note_7_BTN->OnClicked.AddDynamic(this, &ULRInventoryScreenWidget::HandleNote7Clicked);
	Note_8_BTN->OnClicked.AddDynamic(this, &ULRInventoryScreenWidget::HandleNote8Clicked);
	Note_9_BTN->OnClicked.AddDynamic(this, &ULRInventoryScreenWidget::HandleNote9Clicked);
	Note_10_BTN->OnClicked.AddDynamic(this, &ULRInventoryScreenWidget::HandleNote10Clicked);
	Note_11_BTN->OnClicked.AddDynamic(this, &ULRInventoryScreenWidget::HandleNote11Clicked);
	Note_12_BTN->OnClicked.AddDynamic(this, &ULRInventoryScreenWidget::HandleNote12Clicked);
	Col_Icon_1->OnClicked.AddDynamic(this, &ULRInventoryScreenWidget::HandleCollectible1Clicked);
	Col_Icon_2->OnClicked.AddDynamic(this, &ULRInventoryScreenWidget::HandleCollectible2Clicked);
	Col_Icon_3->OnClicked.AddDynamic(this, &ULRInventoryScreenWidget::HandleCollectible3Clicked);
	Col_Icon_4->OnClicked.AddDynamic(this, &ULRInventoryScreenWidget::HandleCollectible4Clicked);
	Col_Icon_5->OnClicked.AddDynamic(this, &ULRInventoryScreenWidget::HandleCollectible5Clicked);
	Col_Icon_6->OnClicked.AddDynamic(this, &ULRInventoryScreenWidget::HandleCollectible6Clicked);
	Col_Icon_7->OnClicked.AddDynamic(this, &ULRInventoryScreenWidget::HandleCollectible7Clicked);
	Col_Icon_8->OnClicked.AddDynamic(this, &ULRInventoryScreenWidget::HandleCollectible8Clicked);
	Col_Icon_9->OnClicked.AddDynamic(this, &ULRInventoryScreenWidget::HandleCollectible9Clicked);
	Col_Icon_10->OnClicked.AddDynamic(this, &ULRInventoryScreenWidget::HandleCollectible10Clicked);
	Col_Icon_11->OnClicked.AddDynamic(this, &ULRInventoryScreenWidget::HandleCollectible11Clicked);
	Col_Icon_12->OnClicked.AddDynamic(this, &ULRInventoryScreenWidget::HandleCollectible12Clicked);
	Choose_Weapon_BTN->OnClicked.AddDynamic(this, &ULRInventoryScreenWidget::HandleChooseWeaponClicked);

	if (Note_Roll)
	{
		Note_Roll->SetIsFocusable(true);
	}
	ApplyTabNavigationGuard();
	UpdateEquipButton();
}

/**
 * @brief 对所有可聚焦控件设置 Slate Next/Previous 导航为 Stop：Tab 键只由 NextTabAction 消费切页，不触发默认焦点遍历。
 */
void ULRInventoryScreenWidget::ApplyTabNavigationGuard()
{
	TArray<UButton*> focusableButtons;
	focusableButtons.Reserve(36);
	if (Bag) { focusableButtons.Add(Bag); }
	if (Note) { focusableButtons.Add(Note); }
	if (Col) { focusableButtons.Add(Col); }
	for (int32 index = 0; index < BagSlotCount; ++index)
	{
		if (UButton* button = GetBagButton(index))
		{
			focusableButtons.Add(button);
		}
	}
	if (Choose_Weapon_BTN)
	{
		focusableButtons.Add(Choose_Weapon_BTN);
	}
	for (int32 index = 0; index < NoteSlotCount; ++index)
	{
		if (UButton* button = GetNoteButton(index))
		{
			focusableButtons.Add(button);
		}
	}
	for (int32 index = 0; index < CollectibleSlotCount; ++index)
	{
		if (UButton* button = GetCollectibleButton(index))
		{
			focusableButtons.Add(button);
		}
	}
	for (UButton* button : focusableButtons)
	{
		button->SetNavigationRuleBase(EUINavigation::Next, EUINavigationRule::Stop);
		button->SetNavigationRuleBase(EUINavigation::Previous, EUINavigationRule::Stop);
	}
}

/**
 * @brief 解除委托并清理计时器或缓存，避免关卡切换和对象销毁后继续收到回调。
 */
void ULRInventoryScreenWidget::NativeDestruct()
{
	CancelPendingFocus();
	if (MenuController)
	{
		MenuController->OnSnapshotChanged.RemoveDynamic(this, &ULRInventoryScreenWidget::HandleSnapshotChanged);
		MenuController = nullptr;
	}
	Super::NativeDestruct();
}

/**
 * @brief 注入统一菜单控制器（由 ALRHUD 在创建 Screen 时调用），并绑定快照变化事件。
 * @param controller 参与本次操作的运行时对象 `controller`；函数会检查空值和所需接口。
 */
void ULRInventoryScreenWidget::SetMenuWidgetController(ULRMenuWidgetController* controller)
{
	if (!controller || MenuController == controller)
	{
		return;
	}
	MenuController = controller;
	MenuController->OnSnapshotChanged.AddDynamic(this, &ULRInventoryScreenWidget::HandleSnapshotChanged);
	// 菜单未打开时控制器已缓存过快照（例如测试或预构建）：直接应用，避免首帧空白。
	if (MenuController->GetCachedSnapshot().bIsValid)
	{
		SetSnapshot(MenuController->GetCachedSnapshot());
	}
}

/**
 * @brief 处理快照变化事件：刷新全部 Tab 表现，清空已消失的选择并重新验证焦点索引。
 * @param snapshot 本次领域操作的结构化数据 `snapshot`；字段语义由对应 USTRUCT 定义。
 */
void ULRInventoryScreenWidget::HandleSnapshotChanged(const FLRInventorySnapshot& snapshot)
{
	SetSnapshot(snapshot);
}

/**
 * @brief 应用新快照并刷新全部 Tab；无效快照让 UI fail closed（全部槽位禁用、详情清空、装备按钮隐藏）。
 * @param snapshot 本次领域操作的结构化数据 `snapshot`；字段语义由对应 USTRUCT 定义。
 */
void ULRInventoryScreenWidget::SetSnapshot(const FLRInventorySnapshot& snapshot)
{
	Snapshot = snapshot;
	if (!Snapshot.bIsValid)
	{
		CurrentBagItemId = NAME_None;
		CurrentReadingId = NAME_None;
		CurrentCollectibleId = NAME_None;
		CachedBagName = CachedBagInfo = CachedNoteName = CachedNoteInfo = CachedCollectibleName = CachedCollectibleInfo = FText();
		UpdateEquipButton();
	}
	RefreshDetailsText();
	RefreshBag();
	RefreshNotes();
	RefreshCollectibles();
	HandleSelectionOutOfDate();
}

/**
 * @brief 按快照刷新背包页：空槽清空 Brush 并 Disabled，有物品时启用并设置图标，武器标识只显示 SelectedWeaponItemId 对应槽位。
 */
void ULRInventoryScreenWidget::RefreshBag()
{
	for (int32 index = 0; index < BagSlotCount; ++index)
	{
		UButton* button = GetBagButton(index);
		UTextBlock* countText = GetBagCountText(index);
		UImage* marker = GetBagWeaponMarker(index);
		if (!button || !countText || !marker)
		{
			continue;
		}
		if (Snapshot.bIsValid && index < Snapshot.Items.Num())
		{
			const FLRInventoryItemView& view = Snapshot.Items[index];
			SetButtonIcon(button, view.Icon);
			button->SetIsEnabled(true);
			countText->SetText(view.bConsumable ? FText::AsNumber(view.Quantity) : FText());
			const bool bSelected = view.ItemId == Snapshot.SelectedWeaponItemId;
			marker->SetVisibility(bSelected ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		}
		else
		{
			SetButtonIcon(button, nullptr);
			button->SetIsEnabled(false);
			countText->SetText(FText());
			marker->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

/**
 * @brief 按快照刷新笔记页：Locked 显示“？？？”并 Disabled，Unlocked 显示标题并可选择。
 */
void ULRInventoryScreenWidget::RefreshNotes()
{
	for (int32 index = 0; index < NoteSlotCount; ++index)
	{
		UButton* button = GetNoteButton(index);
		UTextBlock* titleText = GetNoteTitleText(index);
		if (!button || !titleText)
		{
			continue;
		}
		if (Snapshot.bIsValid && index < Snapshot.Notes.Num())
		{
			const FLRNoteView& view = Snapshot.Notes[index];
			titleText->SetText(view.Title);
			button->SetIsEnabled(view.bUnlocked);
		}
		else
		{
			titleText->SetText(FText());
			button->SetIsEnabled(false);
		}
	}
}

/**
 * @brief 按快照刷新收藏品页：Locked 使用剪影 Brush 并 Disabled，Unlocked 使用真实 Icon 并启用。
 */
void ULRInventoryScreenWidget::RefreshCollectibles()
{
	for (int32 index = 0; index < CollectibleSlotCount; ++index)
	{
		UButton* button = GetCollectibleButton(index);
		if (!button)
		{
			continue;
		}
		if (Snapshot.bIsValid && index < Snapshot.Collectibles.Num())
		{
			const FLRCollectibleView& view = Snapshot.Collectibles[index];
			SetButtonIcon(button, view.Icon);
			button->SetIsEnabled(view.bUnlocked);
		}
		else
		{
			SetButtonIcon(button, nullptr);
			button->SetIsEnabled(false);
		}
	}
}

/**
 * @brief 选择已消失时清空选择、详情缓存并隐藏装备按钮；刷新后重新验证当前 Tab 的焦点索引。
 */
void ULRInventoryScreenWidget::HandleSelectionOutOfDate()
{
	const bool bBagSelectionLost = !CurrentBagItemId.IsNone()
		&& !Snapshot.Items.ContainsByPredicate([this](const FLRInventoryItemView& view)
		{
			return view.ItemId == CurrentBagItemId;
		});
	if (bBagSelectionLost)
	{
		CurrentBagItemId = NAME_None;
		CachedBagName = CachedBagInfo = FText();
	}
	RefreshDetailsText();
	const bool bNoteSelectionLost = !CurrentReadingId.IsNone()
		&& !Snapshot.Notes.ContainsByPredicate([this](const FLRNoteView& view)
		{
			return view.ReadingId == CurrentReadingId;
		});
	if (bNoteSelectionLost)
	{
		CurrentReadingId = NAME_None;
		CachedNoteName = CachedNoteInfo = FText();
	}
	const bool bCollectibleSelectionLost = !CurrentCollectibleId.IsNone()
		&& !Snapshot.Collectibles.ContainsByPredicate([this](const FLRCollectibleView& view)
		{
			return view.CollectibleId == CurrentCollectibleId;
		});
	if (bCollectibleSelectionLost)
	{
		CurrentCollectibleId = NAME_None;
		CachedCollectibleName = CachedCollectibleInfo = FText();
	}
	UpdateEquipButton();

	// 重新验证焦点索引：条目消失或当前焦点落在已禁用槽位上时，next-tick 恢复合法焦点。
	if (!IsScreenVisible())
	{
		return;
	}
	bool bNeedFocusRecovery = false;
	if (CurrentTab == ELRScreenType::Inventory)
	{
		bNeedFocusRecovery = LastBagIndex != INDEX_NONE
			&& (LastBagIndex >= Snapshot.Items.Num() || !GetBagButton(LastBagIndex)->GetIsEnabled());
	}
	else if (CurrentTab == ELRScreenType::Journal)
	{
		bNeedFocusRecovery = LastNoteIndex != INDEX_NONE
			&& (LastNoteIndex >= Snapshot.Notes.Num() || !GetNoteButton(LastNoteIndex)->GetIsEnabled());
	}
	else if (CurrentTab == ELRScreenType::Collectibles)
	{
		bNeedFocusRecovery = LastCollectibleIndex != INDEX_NONE
			&& (LastCollectibleIndex >= Snapshot.Collectibles.Num() || !GetCollectibleButton(LastCollectibleIndex)->GetIsEnabled());
	}
	if (bNeedFocusRecovery)
	{
		RequestFocusRestore();
	}
}

/**
 * @brief 把三个详情区的缓存文本写入 TextBlock；蓝图绑定函数方案因 UMG 绑定数据不可脚本化而由 C++ 直连替代。
 */
void ULRInventoryScreenWidget::RefreshDetailsText()
{
	if (Bag_Item_Name)
	{
		Bag_Item_Name->SetText(CachedBagName);
	}
	if (Bag_Item_Info)
	{
		Bag_Item_Info->SetText(CachedBagInfo);
	}
	if (Note_Name_T)
	{
		Note_Name_T->SetText(CachedNoteName);
	}
	if (Note_Info_T)
	{
		Note_Info_T->SetText(CachedNoteInfo);
	}
	if (Col_Name_T)
	{
		Col_Name_T->SetText(CachedCollectibleName);
	}
	if (Col_Info_T)
	{
		Col_Info_T->SetText(CachedCollectibleInfo);
	}
}

/**
 * @brief 根据当前选择与 Tab 显示/启用 Choose_Weapon_BTN；非武器、非背包 Tab 或关闭菜单时 Collapsed 且 Disabled。
 */
void ULRInventoryScreenWidget::UpdateEquipButton()
{
	if (!Choose_Weapon_BTN)
	{
		return;
	}
	const FLRInventoryItemView* view = Snapshot.bIsValid
		? Snapshot.Items.FindByPredicate([this](const FLRInventoryItemView& item)
		{
			return item.ItemId == CurrentBagItemId;
		})
		: nullptr;
	const bool bShowEquip = CurrentTab == ELRScreenType::Inventory && view && view->bIsWeapon;
	Choose_Weapon_BTN->SetVisibility(bShowEquip ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	Choose_Weapon_BTN->SetIsEnabled(bShowEquip);
}

/**
 * @brief 设置按钮 Normal/Hovered/Pressed Brush；icon 为空时清空为 NoDrawType。
 * @param button 参与本次操作的运行时对象 `button`；函数会检查空值和所需接口。
 * @param icon 参与本次操作的运行时对象 `icon`；函数会检查空值和所需接口。
 */
void ULRInventoryScreenWidget::SetButtonIcon(UButton* button, UTexture2D* icon) const
{
	if (!button)
	{
		return;
	}
	FButtonStyle style = button->WidgetStyle;
	FSlateBrush brush = style.Normal;
	if (icon)
	{
		brush.SetResourceObject(icon);
		if (brush.ImageSize.IsZero())
		{
			brush.ImageSize = FVector2D(64.0f, 64.0f);
		}
	}
	else
	{
		brush.DrawAs = ESlateBrushDrawType::NoDrawType;
		brush.SetResourceObject(nullptr);
	}
	style.Normal = brush;
	style.Hovered = brush;
	style.Pressed = brush;
	button->SetStyle(style);
}

/**
 * @brief 切换统一菜单的当前 Tab（背包/笔记/收藏品）并恢复该 Tab 的会话焦点索引。
 * @param tab 本次操作使用的 `tab` 枚举或模式值；非菜单页值回退到背包页。
 */
void ULRInventoryScreenWidget::SetActiveTab(const ELRScreenType tab)
{
	if (tab == ELRScreenType::Inventory || tab == ELRScreenType::Journal || tab == ELRScreenType::Collectibles)
	{
		CurrentTab = tab;
	}
	else
	{
		CurrentTab = ELRScreenType::Journal;
	}
	if (Content)
	{
		Content->SetActiveWidgetIndex(TabToSwitcherIndex(CurrentTab));
	}
	UpdateEquipButton();
	// Widget 已显示、WidgetSwitcher 已切换：布局完成后的 next-tick 再恢复焦点。
	RequestFocusRestore();
}

/**
 * @brief 解释通用命令：Confirm 确认当前焦点条目、Cancel 关闭菜单、PreviousTab/NextTab 切换页面、PrimaryAction 装备焦点武器。
 * @param command 本次操作使用的 `command` 枚举或模式值。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRInventoryScreenWidget::HandleUICommand_Implementation(const ELRUICommand command)
{
	switch (command)
	{
	case ELRUICommand::Confirm:
	{
		const int32 bagIndex = GetFocusedBagIndex();
		if (bagIndex != INDEX_NONE)
		{
			SelectBagItem(bagIndex);
			return true;
		}
		const int32 noteIndex = GetFocusedNoteIndex();
		if (noteIndex != INDEX_NONE)
		{
			SelectNote(noteIndex);
			return true;
		}
		const int32 collectibleIndex = GetFocusedCollectibleIndex();
		if (collectibleIndex != INDEX_NONE)
		{
			SelectCollectible(collectibleIndex);
			return true;
		}
		if (IsButtonFocused(Choose_Weapon_BTN))
		{
			EquipSelectedWeapon();
			return true;
		}
		return false;
	}
	case ELRUICommand::Cancel:
		CloseMenu();
		return true;
	case ELRUICommand::PreviousTab:
		SetActiveTab(PreviousTabOf(CurrentTab));
		return true;
	case ELRUICommand::NextTab:
		SetActiveTab(NextTabOf(CurrentTab));
		return true;
	case ELRUICommand::PrimaryAction:
		return EquipWeaponAtFocus();
	}
	return false;
}

/**
 * @brief 笔记页的 Note_Roll 聚焦时 Up/Down 按调优步长滚动、Left 返回 LastNoteIndex；其余交给基类 Slate 导航。
 * @param direction 本次输入、状态更新或测试使用的值；二维输入只取绝对值较大的轴。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRInventoryScreenWidget::HandleNavigate(const FVector2D& direction)
{
	if (CurrentTab == ELRScreenType::Journal && IsNoteScrollBoxFocused())
	{
		if (FMath::Abs(direction.X) < FMath::Abs(direction.Y))
		{
			if (!FMath::IsNearlyZero(direction.Y))
			{
				const float step = direction.Y > 0.f ? GetNoteScrollStep() : -GetNoteScrollStep();
				Note_Roll->SetScrollOffset(Note_Roll->GetScrollOffset() + step);
				return true;
			}
		}
		else if (!FMath::IsNearlyZero(direction.X) && direction.X < 0.f)
		{
			return RestoreNoteFocus();
		}
	}
	const bool bNavigated = Super::HandleNavigate(direction);
	if (bNavigated && IsScreenVisible())
	{
		TrackFocusedIndex();
	}
	return bNavigated;
}

/**
 * @brief 恢复当前 Tab 保存的焦点索引；索引无效或条目不可聚焦时失败，由基类回退到 SetInitialFocus。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRInventoryScreenWidget::RestoreFocus()
{
	if (CurrentTab == ELRScreenType::Inventory && RestoreBagFocus())
	{
		return true;
	}
	if (CurrentTab == ELRScreenType::Journal && RestoreNoteFocus())
	{
		return true;
	}
	if (CurrentTab == ELRScreenType::Collectibles && RestoreCollectibleFocus())
	{
		return true;
	}
	return false;
}

/**
 * @brief 设置当前 Tab 的初始焦点：第一个可用（可见、启用、可聚焦）条目；全空时失败，由基类聚焦 Screen 自身。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRInventoryScreenWidget::SetInitialFocus()
{
	if (CurrentTab == ELRScreenType::Inventory && FocusFirstEnabledBagItem())
	{
		return true;
	}
	if (CurrentTab == ELRScreenType::Journal && FocusFirstEnabledNote())
	{
		return true;
	}
	if (CurrentTab == ELRScreenType::Collectibles && FocusFirstEnabledCollectible())
	{
		return true;
	}
	return false;
}

/**
 * @brief 关闭菜单时清空各 Tab 焦点索引、选择 ID 与详情缓存，并取消待执行的焦点请求。
 * @param bVisible 布尔开关 `bVisible`；true 表示启用或条件成立，false 表示禁用或条件不成立。
 */
void ULRInventoryScreenWidget::SetScreenVisible(const bool bVisible)
{
	if (!bVisible && IsScreenVisible())
	{
		CancelPendingFocus();
		LastBagIndex = INDEX_NONE;
		LastNoteIndex = INDEX_NONE;
		LastCollectibleIndex = INDEX_NONE;
		CurrentBagItemId = NAME_None;
		CurrentReadingId = NAME_None;
		CurrentCollectibleId = NAME_None;
		CachedBagName = CachedBagInfo = CachedNoteName = CachedNoteInfo = CachedCollectibleName = CachedCollectibleInfo = FText();
		UpdateEquipButton();
	}
	Super::SetScreenVisible(bVisible);
}

/**
 * @brief 关闭统一菜单；经 PlayerController 回到 PlayerUIComponent 的菜单层仲裁。
 */
void ULRInventoryScreenWidget::CloseMenu()
{
	if (ALRPlayerController* controller = Cast<ALRPlayerController>(GetOwningPlayer()))
	{
		controller->CloseMenuScreen();
	}
}

/**
 * @brief 确认选中背包第 index 格（索引 0 起）：更新当前选择与详情，不装备武器。
 * @param index 本次操作使用的计数、增量或索引 `index`；由函数校验合法范围。
 */
void ULRInventoryScreenWidget::SelectBagItem(const int32 index)
{
	if (!Snapshot.bIsValid || index < 0 || index >= Snapshot.Items.Num())
	{
		return;
	}
	const FLRInventoryItemView& view = Snapshot.Items[index];
	CurrentBagItemId = view.ItemId;
	CachedBagName = view.DisplayName;
	CachedBagInfo = view.Description;
	LastBagIndex = index;
	UpdateEquipButton();
	RefreshDetailsText();
}

/**
 * @brief 确认选中第 index 条笔记（索引 0 起）：更新当前选择与详情。
 * @param index 本次操作使用的计数、增量或索引 `index`；由函数校验合法范围。
 */
void ULRInventoryScreenWidget::SelectNote(const int32 index)
{
	if (!Snapshot.bIsValid || index < 0 || index >= Snapshot.Notes.Num())
	{
		return;
	}
	const FLRNoteView& view = Snapshot.Notes[index];
	if (!view.bUnlocked)
	{
		return;
	}
	CurrentReadingId = view.ReadingId;
	CachedNoteName = view.Title;
	CachedNoteInfo = view.Body;
	LastNoteIndex = index;
	RefreshDetailsText();
}

/**
 * @brief 确认选中第 index 件收藏品（索引 0 起）：更新当前选择与详情。
 * @param index 本次操作使用的计数、增量或索引 `index`；由函数校验合法范围。
 */
void ULRInventoryScreenWidget::SelectCollectible(const int32 index)
{
	if (!Snapshot.bIsValid || index < 0 || index >= Snapshot.Collectibles.Num())
	{
		return;
	}
	const FLRCollectibleView& view = Snapshot.Collectibles[index];
	if (!view.bUnlocked)
	{
		return;
	}
	CurrentCollectibleId = view.CollectibleId;
	CachedCollectibleName = view.DisplayName;
	CachedCollectibleInfo = view.Description;
	LastCollectibleIndex = index;
	RefreshDetailsText();
}

/**
 * @brief 装备当前已确认选择的武器（Choose_Weapon_BTN 点击入口）；合法性由 InventoryComponent 校验。
 */
void ULRInventoryScreenWidget::EquipSelectedWeapon()
{
	if (!MenuController || CurrentBagItemId.IsNone())
	{
		return;
	}
	MenuController->EquipSelectedWeapon(CurrentBagItemId);
}

/**
 * @brief 装备当前焦点所在的背包武器（PrimaryAction 入口）；焦点不在武器格时不装备。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRInventoryScreenWidget::EquipWeaponAtFocus()
{
	if (!MenuController || CurrentTab != ELRScreenType::Inventory)
	{
		return false;
	}
	const int32 index = GetFocusedBagIndex();
	if (index == INDEX_NONE || !Snapshot.bIsValid || index >= Snapshot.Items.Num())
	{
		return false;
	}
	const FLRInventoryItemView& view = Snapshot.Items[index];
	if (!view.bIsWeapon)
	{
		return false;
	}
	return MenuController->EquipSelectedWeapon(view.ItemId);
}

/**
 * @brief 把焦点移动到指定固定按钮并记录该 Tab 的会话焦点索引；目标必须可用。
 * @param index 本次操作使用的计数、增量或索引 `index`；由函数校验合法范围。
 * @param button 参与本次操作的运行时对象 `button`；函数会检查空值和所需接口。
 * @param lastIndex 参与本次操作的运行时对象 `lastIndex`；函数会检查空值和所需接口。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRInventoryScreenWidget::FocusSlot(int32 index, UButton* button, int32& lastIndex)
{
	if (!button || !SetFocusToWidget(button))
	{
		return false;
	}
	lastIndex = index;
	return true;
}

/**
 * @brief 恢复背包页会话焦点索引；索引有效且条目可聚焦时聚焦并返回成功。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRInventoryScreenWidget::RestoreBagFocus()
{
	if (LastBagIndex != INDEX_NONE && LastBagIndex < Snapshot.Items.Num())
	{
		return FocusSlot(LastBagIndex, GetBagButton(LastBagIndex), LastBagIndex);
	}
	return false;
}

/**
 * @brief 恢复笔记页会话焦点索引；索引有效且条目可聚焦时聚焦并返回成功。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRInventoryScreenWidget::RestoreNoteFocus()
{
	if (LastNoteIndex != INDEX_NONE && LastNoteIndex < Snapshot.Notes.Num())
	{
		return FocusSlot(LastNoteIndex, GetNoteButton(LastNoteIndex), LastNoteIndex);
	}
	return false;
}

/**
 * @brief 恢复收藏品页会话焦点索引；索引有效且条目可聚焦时聚焦并返回成功。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRInventoryScreenWidget::RestoreCollectibleFocus()
{
	if (LastCollectibleIndex != INDEX_NONE && LastCollectibleIndex < Snapshot.Collectibles.Num())
	{
		return FocusSlot(LastCollectibleIndex, GetCollectibleButton(LastCollectibleIndex), LastCollectibleIndex);
	}
	return false;
}

/**
 * @brief 聚焦背包页第一个可用条目。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRInventoryScreenWidget::FocusFirstEnabledBagItem()
{
	for (int32 index = 0; index < BagSlotCount && index < Snapshot.Items.Num(); ++index)
	{
		UButton* button = GetBagButton(index);
		if (button && button->GetIsEnabled() && button->IsVisible() && button->SupportsKeyboardFocus())
		{
			return FocusSlot(index, button, LastBagIndex);
		}
	}
	return false;
}

/**
 * @brief 聚焦笔记页第一个可用条目。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRInventoryScreenWidget::FocusFirstEnabledNote()
{
	for (int32 index = 0; index < NoteSlotCount && index < Snapshot.Notes.Num(); ++index)
	{
		UButton* button = GetNoteButton(index);
		if (button && button->GetIsEnabled() && button->IsVisible() && button->SupportsKeyboardFocus())
		{
			return FocusSlot(index, button, LastNoteIndex);
		}
	}
	return false;
}

/**
 * @brief 聚焦收藏品页第一个可用条目。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRInventoryScreenWidget::FocusFirstEnabledCollectible()
{
	for (int32 index = 0; index < CollectibleSlotCount && index < Snapshot.Collectibles.Num(); ++index)
	{
		UButton* button = GetCollectibleButton(index);
		if (button && button->GetIsEnabled() && button->IsVisible() && button->SupportsKeyboardFocus())
		{
			return FocusSlot(index, button, LastCollectibleIndex);
		}
	}
	return false;
}

/**
 * @brief 在布局完成后的 next-tick 恢复当前 Tab 的会话焦点；关闭界面时通过 SetScreenVisible(false) 取消。
 */
void ULRInventoryScreenWidget::RequestFocusRestore()
{
	CancelPendingFocus();
	if (UWorld* world = GetWorld())
	{
		world->GetTimerManager().SetTimerForNextTick(this, &ULRInventoryScreenWidget::HandlePendingFocusRestore);
	}
}

/**
 * @brief 取消待执行的 next-tick 焦点请求。
 */
void ULRInventoryScreenWidget::CancelPendingFocus()
{
	if (UWorld* world = GetWorld())
	{
		world->GetTimerManager().ClearTimer(PendingFocusTimer);
	}
}

/**
 * @brief 执行待执行的焦点恢复回调。
 */
void ULRInventoryScreenWidget::HandlePendingFocusRestore()
{
	if (IsScreenVisible())
	{
		RestoreFocus();
	}
}

/**
 * @brief 判断当前用户焦点是否落在笔记列表 ScrollBox 上。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRInventoryScreenWidget::IsNoteScrollBoxFocused() const
{
	if (!Note_Roll)
	{
		return false;
	}
	const TSharedPtr<SWidget> currentFocus = FSlateApplication::Get().GetUserFocusedWidget(GetSlateUserIndex());
	return currentFocus.IsValid() && currentFocus == Note_Roll->GetCachedWidget();
}

/**
 * @brief 查询笔记列表滚动步长；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
float ULRInventoryScreenWidget::GetNoteScrollStep() const
{
	if (const ULRGameInstanceSubsystem* subsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr)
	{
		if (const ULRGameTuningSet* tuning = subsystem->GetTuningSet())
		{
			return tuning->UI ? tuning->UI->NoteScrollStep : 60.0f;
		}
	}
	return 60.0f;
}

/**
 * @brief 查询当前用户焦点对应的背包槽索引（0 起）；不在背包槽位上时返回 -1。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
int32 ULRInventoryScreenWidget::GetFocusedBagIndex() const
{
	const TSharedPtr<SWidget> currentFocus = FSlateApplication::Get().GetUserFocusedWidget(GetSlateUserIndex());
	for (int32 index = 0; index < BagSlotCount; ++index)
	{
		const UButton* button = GetBagButton(index);
		if (button && button->GetCachedWidget() == currentFocus)
		{
			return index;
		}
	}
	return INDEX_NONE;
}

/**
 * @brief 查询当前用户焦点对应的笔记槽索引（0 起）；不在笔记槽位上时返回 -1。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
int32 ULRInventoryScreenWidget::GetFocusedNoteIndex() const
{
	const TSharedPtr<SWidget> currentFocus = FSlateApplication::Get().GetUserFocusedWidget(GetSlateUserIndex());
	for (int32 index = 0; index < NoteSlotCount; ++index)
	{
		const UButton* button = GetNoteButton(index);
		if (button && button->GetCachedWidget() == currentFocus)
		{
			return index;
		}
	}
	return INDEX_NONE;
}

/**
 * @brief 查询当前用户焦点对应的收藏品槽索引（0 起）；不在收藏品槽位上时返回 -1。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
int32 ULRInventoryScreenWidget::GetFocusedCollectibleIndex() const
{
	const TSharedPtr<SWidget> currentFocus = FSlateApplication::Get().GetUserFocusedWidget(GetSlateUserIndex());
	for (int32 index = 0; index < CollectibleSlotCount; ++index)
	{
		const UButton* button = GetCollectibleButton(index);
		if (button && button->GetCachedWidget() == currentFocus)
		{
			return index;
		}
	}
	return INDEX_NONE;
}

/**
 * @brief 判断当前用户焦点是否落在指定按钮上。
 * @param button 参与本次操作的运行时对象 `button`；函数会检查空值和所需接口。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRInventoryScreenWidget::IsButtonFocused(const UButton* button) const
{
	return button && button->GetCachedWidget() == FSlateApplication::Get().GetUserFocusedWidget(GetSlateUserIndex());
}

/**
 * @brief 记录当前焦点所在的固定槽位索引；焦点不在任何槽位时保持原值。
 */
void ULRInventoryScreenWidget::TrackFocusedIndex()
{
	const int32 bagIndex = GetFocusedBagIndex();
	if (bagIndex != INDEX_NONE)
	{
		LastBagIndex = bagIndex;
		return;
	}
	const int32 noteIndex = GetFocusedNoteIndex();
	if (noteIndex != INDEX_NONE)
	{
		LastNoteIndex = noteIndex;
		return;
	}
	const int32 collectibleIndex = GetFocusedCollectibleIndex();
	if (collectibleIndex != INDEX_NONE)
	{
		LastCollectibleIndex = collectibleIndex;
	}
}

/** 固定背包按钮点击：确认对应槽位。 */
void ULRInventoryScreenWidget::HandleBagItem1Clicked() { SelectBagItem(0); }
void ULRInventoryScreenWidget::HandleBagItem2Clicked() { SelectBagItem(1); }
void ULRInventoryScreenWidget::HandleBagItem3Clicked() { SelectBagItem(2); }
void ULRInventoryScreenWidget::HandleBagItem4Clicked() { SelectBagItem(3); }
void ULRInventoryScreenWidget::HandleBagItem5Clicked() { SelectBagItem(4); }
void ULRInventoryScreenWidget::HandleBagItem6Clicked() { SelectBagItem(5); }
void ULRInventoryScreenWidget::HandleBagItem7Clicked() { SelectBagItem(6); }
void ULRInventoryScreenWidget::HandleBagItem8Clicked() { SelectBagItem(7); }

/** 固定笔记按钮点击：确认对应条目。 */
void ULRInventoryScreenWidget::HandleNote1Clicked() { SelectNote(0); }
void ULRInventoryScreenWidget::HandleNote2Clicked() { SelectNote(1); }
void ULRInventoryScreenWidget::HandleNote3Clicked() { SelectNote(2); }
void ULRInventoryScreenWidget::HandleNote4Clicked() { SelectNote(3); }
void ULRInventoryScreenWidget::HandleNote5Clicked() { SelectNote(4); }
void ULRInventoryScreenWidget::HandleNote6Clicked() { SelectNote(5); }
void ULRInventoryScreenWidget::HandleNote7Clicked() { SelectNote(6); }
void ULRInventoryScreenWidget::HandleNote8Clicked() { SelectNote(7); }
void ULRInventoryScreenWidget::HandleNote9Clicked() { SelectNote(8); }
void ULRInventoryScreenWidget::HandleNote10Clicked() { SelectNote(9); }
void ULRInventoryScreenWidget::HandleNote11Clicked() { SelectNote(10); }
void ULRInventoryScreenWidget::HandleNote12Clicked() { SelectNote(11); }

/** 固定收藏品按钮点击：确认对应条目。 */
void ULRInventoryScreenWidget::HandleCollectible1Clicked() { SelectCollectible(0); }
void ULRInventoryScreenWidget::HandleCollectible2Clicked() { SelectCollectible(1); }
void ULRInventoryScreenWidget::HandleCollectible3Clicked() { SelectCollectible(2); }
void ULRInventoryScreenWidget::HandleCollectible4Clicked() { SelectCollectible(3); }
void ULRInventoryScreenWidget::HandleCollectible5Clicked() { SelectCollectible(4); }
void ULRInventoryScreenWidget::HandleCollectible6Clicked() { SelectCollectible(5); }
void ULRInventoryScreenWidget::HandleCollectible7Clicked() { SelectCollectible(6); }
void ULRInventoryScreenWidget::HandleCollectible8Clicked() { SelectCollectible(7); }
void ULRInventoryScreenWidget::HandleCollectible9Clicked() { SelectCollectible(8); }
void ULRInventoryScreenWidget::HandleCollectible10Clicked() { SelectCollectible(9); }
void ULRInventoryScreenWidget::HandleCollectible11Clicked() { SelectCollectible(10); }
void ULRInventoryScreenWidget::HandleCollectible12Clicked() { SelectCollectible(11); }

/** 装备按钮点击：装备已确认选择的武器。 */
void ULRInventoryScreenWidget::HandleChooseWeaponClicked()
{
	EquipSelectedWeapon();
}

/** 按索引返回固定背包按钮；越界返回空。 */
UButton* ULRInventoryScreenWidget::GetBagButton(const int32 index) const
{
	switch (index)
	{
	case 0: return Bag_Item_BTN_1;
	case 1: return Bag_Item_BTN_2;
	case 2: return Bag_Item_BTN_3;
	case 3: return Bag_Item_BTN_4;
	case 4: return Bag_Item_BTN_5;
	case 5: return Bag_Item_BTN_6;
	case 6: return Bag_Item_BTN_7;
	case 7: return Bag_Item_BTN_8;
	default: return nullptr;
	}
}

/** 按索引返回固定笔记按钮；越界返回空。 */
UButton* ULRInventoryScreenWidget::GetNoteButton(const int32 index) const
{
	switch (index)
	{
	case 0: return Note_1_BTN;
	case 1: return Note_2_BTN;
	case 2: return Note_3_BTN;
	case 3: return Note_4_BTN;
	case 4: return Note_5_BTN;
	case 5: return Note_6_BTN;
	case 6: return Note_7_BTN;
	case 7: return Note_8_BTN;
	case 8: return Note_9_BTN;
	case 9: return Note_10_BTN;
	case 10: return Note_11_BTN;
	case 11: return Note_12_BTN;
	default: return nullptr;
	}
}

/** 按索引返回固定收藏品按钮；越界返回空。 */
UButton* ULRInventoryScreenWidget::GetCollectibleButton(const int32 index) const
{
	switch (index)
	{
	case 0: return Col_Icon_1;
	case 1: return Col_Icon_2;
	case 2: return Col_Icon_3;
	case 3: return Col_Icon_4;
	case 4: return Col_Icon_5;
	case 5: return Col_Icon_6;
	case 6: return Col_Icon_7;
	case 7: return Col_Icon_8;
	case 8: return Col_Icon_9;
	case 9: return Col_Icon_10;
	case 10: return Col_Icon_11;
	case 11: return Col_Icon_12;
	default: return nullptr;
	}
}

/** 按索引返回固定背包数量文本；越界返回空。 */
UTextBlock* ULRInventoryScreenWidget::GetBagCountText(const int32 index) const
{
	switch (index)
	{
	case 0: return Bag_Item_Cnt_1;
	case 1: return Bag_Item_Cnt_2;
	case 2: return Bag_Item_Cnt_3;
	case 3: return Bag_Item_Cnt_4;
	case 4: return Bag_Item_Cnt_5;
	case 5: return Bag_Item_Cnt_6;
	case 6: return Bag_Item_Cnt_7;
	case 7: return Bag_Item_Cnt_8;
	default: return nullptr;
	}
}

/** 按索引返回固定笔记标题文本；越界返回空。 */
UTextBlock* ULRInventoryScreenWidget::GetNoteTitleText(const int32 index) const
{
	switch (index)
	{
	case 0: return Note_1_T;
	case 1: return Note_2_T;
	case 2: return Note_3_T;
	case 3: return Note_4_T;
	case 4: return Note_5_T;
	case 5: return Note_6_T;
	case 6: return Note_7_T;
	case 7: return Note_8_T;
	case 8: return Note_9_T;
	case 9: return Note_10_T;
	case 10: return Note_11_T;
	case 11: return Note_12_T;
	default: return nullptr;
	}
}

/** 按索引返回固定武器标识 Image；越界返回空。 */
UImage* ULRInventoryScreenWidget::GetBagWeaponMarker(const int32 index) const
{
	switch (index)
	{
	case 0: return Bag_Weapon_1;
	case 1: return Bag_Weapon_2;
	case 2: return Bag_Weapon_3;
	case 3: return Bag_Weapon_4;
	case 4: return Bag_Weapon_5;
	case 5: return Bag_Weapon_6;
	case 6: return Bag_Weapon_7;
	case 7: return Bag_Weapon_8;
	default: return nullptr;
	}
}
