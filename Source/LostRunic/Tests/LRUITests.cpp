/**
 * @file LRUITests.cpp
 * @brief 提供 LostRunic UI 自动化测试：快照排序、容量契约、领域事件、Locked 不泄露、通用命令路由、输入配置校验与输入层仲裁。仅在 WITH_DEV_AUTOMATION_TESTS 下编译。
 *
 * 关联文件：Tests/LRTestUIHelpers.h；所属领域：Tests。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/LRGameplayTags.h"
#include "Data/LRCollectibleDefinition.h"
#include "Data/LRContentRows.h"
#include "Data/LRGameContentSet.h"
#include "Data/LRItemDefinition.h"
#include "Data/LRProjectSettings.h"
#include "Data/LRUITuning.h"
#include "Engine/Texture2D.h"
#include "EnhancedActionKeyMapping.h"
#include "Input/LRInputConfig.h"
#include "Items/LRInventoryComponent.h"
#include "Save/LRSaveTypes.h"
#include "Save/LRSaveFormatting.h"
#include "Tests/LRTestUIHelpers.h"
#include "UI/LRHUD.h"
#include "UI/LRInventoryScreenWidget.h"
#include "UI/LRMenuWidgetController.h"
#include "UI/LRPlayerUIComponent.h"
#include "UI/LRSaveWidgetController.h"
#include "InputAction.h"
#include "InputMappingContext.h"

namespace
{
	/**
	 * @brief 构造带唯一 ID 的测试物品定义。
	 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	ULRItemDefinition* MakeItem(const FName itemId)
	{
		ULRItemDefinition* definition = NewObject<ULRItemDefinition>();
		definition->ItemId = itemId;
		return definition;
	}

	/**
	 * @brief 构造带指定 ID 的测试收藏品定义。
	 * @param collectibleId 稳定标识 `collectibleId`；用于内容查询和存档，不依赖显示名或数组序号。
	 * @param displayOrder 本次操作使用的计数、增量或索引 `displayOrder`；由函数校验合法范围。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	ULRCollectibleDefinition* MakeCollectible(const FName collectibleId, const int32 displayOrder)
	{
		ULRCollectibleDefinition* definition = NewObject<ULRCollectibleDefinition>();
		definition->CollectibleId = collectibleId;
		definition->DisplayOrder = displayOrder;
		return definition;
	}

	/**
	 * @brief 构造指定 ReadingId 的阅读表行；行名与 ReadingId 一致。
	 * @param readingId 稳定标识 `readingId`；用于内容查询和存档，不依赖显示名或数组序号。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	FLRReadingRow MakeReadingRow(const FName readingId)
	{
		FLRReadingRow row;
		row.ReadingId = readingId;
		row.Title = FText::FromName(readingId);
		return row;
	}

	/**
	 * @brief 构造包含全部必填引用的测试输入配置；bIncludeNewActions 控制新增 UI 动作是否齐备。
	 * @param bIncludeNewActions 布尔开关 `bIncludeNewActions`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	ULRInputConfig* MakeInputConfig(const bool bIncludeNewActions)
	{
		ULRInputConfig* config = NewObject<ULRInputConfig>();
		config->GameplayContext = NewObject<UInputMappingContext>();
		config->DialogueContext = NewObject<UInputMappingContext>();
		config->MenuContext = NewObject<UInputMappingContext>();
		config->TransitionContext = NewObject<UInputMappingContext>();
		config->MoveAction = NewObject<UInputAction>();
		config->SneakAction = NewObject<UInputAction>();
		config->RunAction = NewObject<UInputAction>();
		config->InteractAction = NewObject<UInputAction>();
		config->CloseEyesAction = NewObject<UInputAction>();
		config->OpenEyesAction = NewObject<UInputAction>();
		config->ConfirmAction = NewObject<UInputAction>();
		config->CancelAction = NewObject<UInputAction>();
		config->AttackAction = NewObject<UInputAction>();
		config->ToggleCrouchAction = NewObject<UInputAction>();
		config->OpenInventoryAction = NewObject<UInputAction>();
		config->PauseAction = NewObject<UInputAction>();
		if (bIncludeNewActions)
		{
			config->NavigateAction = NewObject<UInputAction>();
			config->PreviousTabAction = NewObject<UInputAction>();
			config->NextTabAction = NewObject<UInputAction>();
			config->UIPrimaryAction = NewObject<UInputAction>();
			config->UIDeleteAction = NewObject<UInputAction>();
		}
		return config;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRMenuSnapshotOrderingTest, "LostRunic.UI.MenuSnapshotOrdersEntries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRMenuSnapshotOrderingTest::RunTest(const FString& parameters)
{
	// 背包：按获得顺序（AcquisitionSequence）再按 ItemId。
	ULRItemDefinition* itemB = MakeItem(TEXT("Test.B"));
	ULRItemDefinition* itemA = MakeItem(TEXT("Test.A"));
	ULRItemDefinition* itemC = MakeItem(TEXT("Test.C"));
	ULRInventoryComponent* inventory = NewObject<ULRInventoryComponent>();
	inventory->InitializeDefinitions({ itemB, itemA, itemC });
	inventory->AddItem(itemB->ItemId, 1);
	inventory->AddItem(itemA->ItemId, 1);
	inventory->AddItem(itemC->ItemId, 1);

	const TArray<FLRInventoryEntry> ordered = inventory->GetOwnedEntriesOrdered();
	TestEqual(TEXT("Entries are ordered by acquisition sequence"), ordered.Num(), 3);
	if (ordered.Num() == 3)
	{
		TestEqual(TEXT("First acquired item first"), ordered[0].ItemId, itemB->ItemId);
		TestEqual(TEXT("Second acquired item second"), ordered[1].ItemId, itemA->ItemId);
		TestEqual(TEXT("Third acquired item third"), ordered[2].ItemId, itemC->ItemId);
	}

	// 笔记：ReadingId 字典序（含 Locked 占位）。
	ULRGameContentSet* contentSet = NewObject<ULRGameContentSet>();
	contentSet->Collectibles = {
		MakeCollectible(TEXT("Col.High"), 2),
		MakeCollectible(TEXT("Col.Low"), 0),
		MakeCollectible(TEXT("Col.Mid"), 1)
	};
	ULRMenuWidgetController* controller = NewObject<ULRMenuWidgetController>();
	controller->Initialize(inventory, contentSet, nullptr);

	FLRInventorySnapshot snapshot = controller->BuildInventorySnapshot(inventory);
	TestEqual(TEXT("Snapshot bag follows acquisition order"), snapshot.Items.Num(), 3);
	if (snapshot.Items.Num() == 3)
	{
		TestEqual(TEXT("Snapshot bag first item"), snapshot.Items[0].ItemId, itemB->ItemId);
		TestEqual(TEXT("Snapshot bag second item"), snapshot.Items[1].ItemId, itemA->ItemId);
		TestEqual(TEXT("Snapshot bag third item"), snapshot.Items[2].ItemId, itemC->ItemId);
	}
	TArray<FLRNoteView> notes;
	ULRMenuWidgetController::BuildNoteViews(
		{ MakeReadingRow(TEXT("Note.C")), MakeReadingRow(TEXT("Note.A")), MakeReadingRow(TEXT("Note.B")) },
		*inventory, notes);
	TestEqual(TEXT("Notes include locked placeholders"), notes.Num(), 3);
	if (notes.Num() == 3)
	{
		TestEqual(TEXT("Notes sorted by ReadingId"), notes[0].ReadingId, TEXT("Note.A"));
		TestEqual(TEXT("Notes sorted by ReadingId"), notes[1].ReadingId, TEXT("Note.B"));
		TestEqual(TEXT("Notes sorted by ReadingId"), notes[2].ReadingId, TEXT("Note.C"));
		TestFalse(TEXT("Unread notes stay locked"), notes[0].bUnlocked);
	}

	// 收藏品：DisplayOrder 再按 CollectibleId。
	TestEqual(TEXT("Collectibles sorted by DisplayOrder"), snapshot.Collectibles.Num(), 3);
	if (snapshot.Collectibles.Num() == 3)
	{
		TestEqual(TEXT("Collectible order 0"), snapshot.Collectibles[0].CollectibleId, TEXT("Col.Low"));
		TestEqual(TEXT("Collectible order 1"), snapshot.Collectibles[1].CollectibleId, TEXT("Col.Mid"));
		TestEqual(TEXT("Collectible order 2"), snapshot.Collectibles[2].CollectibleId, TEXT("Col.High"));
	}
	TestTrue(TEXT("Within-capacity snapshot is valid"), snapshot.bIsValid);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRInventoryCapacityTest, "LostRunic.Inventory.NinthDistinctItemRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRInventoryCapacityTest::RunTest(const FString& parameters)
{
	ULRInventoryComponent* inventory = NewObject<ULRInventoryComponent>();
	TArray<ULRItemDefinition*> definitions;
	for (int32 index = 0; index < 9; ++index)
	{
		definitions.Add(MakeItem(FName(*FString::Printf(TEXT("Test.Item%02d"), index + 1))));
	}
	definitions[0]->MaxStackSize = 99;
	inventory->InitializeDefinitions(definitions);

	for (int32 index = 0; index < 8; ++index)
	{
		TestEqual(TEXT("First eight distinct items are accepted"),
			inventory->AddItem(definitions[index]->ItemId, 1), ELRAddItemResult::Success);
	}
	// 已有条目堆叠不受容量限制。
	TestEqual(TEXT("Stacking an existing item stays legal"),
		inventory->AddItem(definitions[0]->ItemId, 1), ELRAddItemResult::Success);
	TestEqual(TEXT("Ninth distinct item is rejected with InventoryFull"),
		inventory->AddItem(definitions[8]->ItemId, 1), ELRAddItemResult::InventoryFull);
	TestEqual(TEXT("Ninth item was not added"), inventory->GetItemCount(definitions[8]->ItemId), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRWeaponEventTest, "LostRunic.Inventory.WeaponSelectionEvents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRWeaponEventTest::RunTest(const FString& parameters)
{
	ULRItemDefinition* knife = MakeItem(TEXT("Test.Knife"));
	knife->bConsumable = true;
	knife->MaxStackSize = 1;
	knife->ItemTags.AddTag(LRGameplayTags::ItemCategoryWeapon);
	ULRInventoryComponent* inventory = NewObject<ULRInventoryComponent>();
	inventory->InitializeDefinitions({ knife });
	inventory->AddItem(knife->ItemId, 1);

	ULRTestInventoryObserver* observer = NewObject<ULRTestInventoryObserver>();
	inventory->OnSelectedWeaponChanged.AddDynamic(observer, &ULRTestInventoryObserver::HandleSelectedWeaponChanged);
	inventory->OnNotesChanged.AddDynamic(observer, &ULRTestInventoryObserver::HandleNotesChanged);
	inventory->OnCollectiblesChanged.AddDynamic(observer, &ULRTestInventoryObserver::HandleCollectiblesChanged);

	TestTrue(TEXT("Equipping a held weapon succeeds"), inventory->SetSelectedWeapon(knife->ItemId));
	TestEqual(TEXT("Equip broadcasts weapon event"), observer->SelectedWeaponChangedCount, 1);
	TestTrue(TEXT("Re-equipping the same weapon is a no-op event"),
		[&]()
		{
			const int32 before = observer->SelectedWeaponChangedCount;
			inventory->SetSelectedWeapon(knife->ItemId);
			return observer->SelectedWeaponChangedCount == before;
		}());
	TestEqual(TEXT("Clearing selection broadcasts weapon event"),
		[&]()
		{
			inventory->SetSelectedWeapon(NAME_None);
			return observer->SelectedWeaponChangedCount;
		}(), 2);

	// 存档恢复广播笔记、收藏品与武器事件。
	FLRSaveNotebookChunk chunk;
	chunk.NoteIds.Add(TEXT("Note.Restored"));
	inventory->RestoreNotebookSaveState(chunk);
	TestEqual(TEXT("Restore broadcasts notes event"), observer->NotesChangedCount, 1);
	TestEqual(TEXT("Restore broadcasts collectibles event"), observer->CollectiblesChangedCount, 1);
	TestEqual(TEXT("Restore broadcasts weapon event"), observer->SelectedWeaponChangedCount, 3);
	TestEqual(TEXT("Restored notes are owned"), inventory->GetNoteIds().Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRLockedSnapshotTest, "LostRunic.UI.LockedViewsDoNotLeakContent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRLockedSnapshotTest::RunTest(const FString& parameters)
{
	UTexture2D* realIcon = NewObject<UTexture2D>();
	UTexture2D* perDefinitionSilhouette = NewObject<UTexture2D>();
	UTexture2D* sharedSilhouette = NewObject<UTexture2D>();

	ULRCollectibleDefinition* doll = MakeCollectible(TEXT("Col.Doll"), 0);
	doll->DisplayName = FText::FromString(TEXT("Doll"));
	doll->Description = FText::FromString(TEXT("A memory of the home."));
	doll->Icon = TSoftObjectPtr<UTexture2D>(realIcon);
	doll->LockedIcon = TSoftObjectPtr<UTexture2D>(perDefinitionSilhouette);

	ULRGameContentSet* contentSet = NewObject<ULRGameContentSet>();
	contentSet->Collectibles = { doll };

	ULRUITuning* uiTuning = NewObject<ULRUITuning>();
	uiTuning->LockedCollectibleIcon = TSoftObjectPtr<UTexture2D>(sharedSilhouette);

	ULRInventoryComponent* inventory = NewObject<ULRInventoryComponent>();
	inventory->InitializeCollectibleDefinitions({ doll });
	inventory->AddNoteId(TEXT("Note.Mother"));

	ULRMenuWidgetController* controller = NewObject<ULRMenuWidgetController>();
	controller->Initialize(inventory, contentSet, uiTuning);
	const FLRInventorySnapshot snapshot = controller->BuildInventorySnapshot(inventory);

	// 笔记 Locked 语义：已读暴露真实标题，未读只暴露“？？？”。
	TArray<FLRNoteView> notes;
	ULRMenuWidgetController::BuildNoteViews(
		{ MakeReadingRow(TEXT("Note.Mother")), MakeReadingRow(TEXT("Note.Secret")) }, *inventory, notes);
	const FLRNoteView* unlockedNote = notes.FindByPredicate(
		[](const FLRNoteView& view) { return view.ReadingId == TEXT("Note.Mother"); });
	TestNotNull(TEXT("Unlocked note exists"), unlockedNote);
	if (unlockedNote)
	{
		TestTrue(TEXT("Unlocked note is unlocked"), unlockedNote->bUnlocked);
		TestEqual(TEXT("Unlocked note exposes real title"), unlockedNote->Title.ToString(), TEXT("Note.Mother"));
	}
	const FLRNoteView* lockedNote = notes.FindByPredicate(
		[](const FLRNoteView& view) { return view.ReadingId == TEXT("Note.Secret"); });
	TestNotNull(TEXT("Locked note exists"), lockedNote);
	if (lockedNote)
	{
		TestFalse(TEXT("Locked note is not unlocked"), lockedNote->bUnlocked);
		TestEqual(TEXT("Locked note title is question marks"), lockedNote->Title.ToString(), TEXT("？？？"));
		TestTrue(TEXT("Locked note body stays empty"), lockedNote->Body.IsEmpty());
	}

	// 收藏品未解锁：不暴露真实名称/描述，只暴露剪影图。
	TestEqual(TEXT("Locked collectible hides real name"), snapshot.Collectibles.Num(), 1);
	if (snapshot.Collectibles.Num() == 1)
	{
		const FLRCollectibleView& lockedView = snapshot.Collectibles[0];
		TestFalse(TEXT("Collectible starts locked"), lockedView.bUnlocked);
		TestTrue(TEXT("Collectible hides real name"), lockedView.DisplayName.IsEmpty());
		TestTrue(TEXT("Collectible hides real description"), lockedView.Description.IsEmpty());
		TestEqual(TEXT("Collectible exposes per-definition silhouette"), lockedView.Icon.Get(), perDefinitionSilhouette);

		inventory->AddCollectibleId(TEXT("Col.Doll"));
		const FLRInventorySnapshot refreshed = controller->BuildInventorySnapshot(inventory);
		TestEqual(TEXT("Refreshed collectible count"), refreshed.Collectibles.Num(), 1);
		if (refreshed.Collectibles.Num() == 1)
		{
			TestTrue(TEXT("Collectible unlocks after ownership"), refreshed.Collectibles[0].bUnlocked);
			TestEqual(TEXT("Unlocked collectible exposes real name"), refreshed.Collectibles[0].DisplayName.ToString(), TEXT("Doll"));
			TestEqual(TEXT("Unlocked collectible exposes real icon"), refreshed.Collectibles[0].Icon.Get(), realIcon);
		}
	}

	// 定义缺失 LockedIcon 时回退到共享剪影。
	ULRCollectibleDefinition* second = MakeCollectible(TEXT("Col.Letter"), 1);
	contentSet->Collectibles = { doll, second };
	const FLRInventorySnapshot withShared = controller->BuildInventorySnapshot(inventory);
	const FLRCollectibleView* sharedView = withShared.Collectibles.FindByPredicate(
		[](const FLRCollectibleView& view) { return view.CollectibleId == TEXT("Col.Letter"); });
	TestNotNull(TEXT("Second collectible exists"), sharedView);
	if (sharedView)
	{
		TestEqual(TEXT("Missing LockedIcon falls back to shared silhouette"), sharedView->Icon.Get(), sharedSilhouette);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRSnapshotOverflowTest, "LostRunic.UI.SnapshotOverflowFailsClosed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRSnapshotOverflowTest::RunTest(const FString& parameters)
{
	// 存档恢复不设容量门，可构造 13 条笔记的越界库存；快照必须 fail closed。
	ULRInventoryComponent* inventory = NewObject<ULRInventoryComponent>();
	FLRSaveNotebookChunk chunk;
	for (int32 index = 0; index < 13; ++index)
	{
		chunk.NoteIds.Add(FName(*FString::Printf(TEXT("Note.%02d"), index + 1)));
	}
	inventory->RestoreNotebookSaveState(chunk);
	TestEqual(TEXT("Restored thirteen notes"), inventory->GetNoteIds().Num(), 13);

	ULRMenuWidgetController* controller = NewObject<ULRMenuWidgetController>();
	AddExpectedError(TEXT("Menu snapshot invalid"), EAutomationExpectedErrorFlags::Contains, 1);
	const FLRInventorySnapshot snapshot = controller->BuildInventorySnapshot(inventory);
	TestFalse(TEXT("Over-capacity snapshot is invalid"), snapshot.bIsValid);
	TestTrue(TEXT("Invalid snapshot never partially shows items"), snapshot.Items.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRUICommandRoutingTest, "LostRunic.UI.BaseScreenDoesNotHandleInventoryCommands",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRUICommandRoutingTest::RunTest(const FString& parameters)
{
	ULRTestScreenWidget* screen = NewObject<ULRTestScreenWidget>();
	screen->SetScreenVisible(true);
	TestTrue(TEXT("Screen is visible after show"), screen->IsScreenVisible());

	// 基类不理解任何具体页面命令：全部返回未处理，证明命令路由不依赖 Inventory 类型。
	TestFalse(TEXT("Base screen does not handle Confirm"), screen->HandleUICommand(ELRUICommand::Confirm));
	TestFalse(TEXT("Base screen does not handle Cancel"), screen->HandleUICommand(ELRUICommand::Cancel));
	TestFalse(TEXT("Base screen does not handle PreviousTab"), screen->HandleUICommand(ELRUICommand::PreviousTab));
	TestFalse(TEXT("Base screen does not handle NextTab"), screen->HandleUICommand(ELRUICommand::NextTab));
	TestFalse(TEXT("Base screen does not handle PrimaryAction"), screen->HandleUICommand(ELRUICommand::PrimaryAction));

	// 零方向不产生导航。
	TestFalse(TEXT("Zero direction never navigates"), screen->HandleNavigate(FVector2D::ZeroVector));

	// 焦点失效时故障恢复仍能落到“至少一个合法焦点”（Screen 自身），不泄漏到 Gameplay。
	TestTrue(TEXT("Invalid focus recovers to a valid focus target"), screen->HandleNavigate(FVector2D(1.0f, 0.0f)));

	// 隐藏后拒绝导航输入。
	screen->SetScreenVisible(false);
	TestFalse(TEXT("Hidden screen rejects navigation"), screen->HandleNavigate(FVector2D(1.0f, 0.0f)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRUIInputConfigValidationTest, "LostRunic.Input.UIValidationRequiresNewActions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRUIInputConfigValidationTest::RunTest(const FString& parameters)
{
	FString error;
	TestFalse(TEXT("Missing new UI actions fails validation"), MakeInputConfig(false)->Validate(error));
	TestTrue(TEXT("Error message names the missing group"), error.Contains(TEXT("UI navigation actions")));

	ULRInputConfig* complete = MakeInputConfig(true);
	TestTrue(TEXT("All UI actions assigned passes validation"), complete->Validate(error));

	complete->NavigateAction = nullptr;
	TestFalse(TEXT("Missing NavigateAction fails validation"), complete->Validate(error));
	complete->NavigateAction = NewObject<UInputAction>();
	complete->UIPrimaryAction = nullptr;
	TestFalse(TEXT("Missing UIPrimaryAction fails validation"), complete->Validate(error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRLayerArbitrationTest, "LostRunic.UI.InputLayerPriorityAndRestore",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRLayerArbitrationTest::RunTest(const FString& parameters)
{
	// 纯函数优先级：Transition > Dialogue > Menu > Gameplay。
	TestEqual(TEXT("All layers off resolves Gameplay"),
		ULRPlayerUIComponent::ComputeEffectiveInputMode(false, false, false), ELRInputMode::Gameplay);
	TestEqual(TEXT("Menu only resolves Menu"),
		ULRPlayerUIComponent::ComputeEffectiveInputMode(false, false, true), ELRInputMode::Menu);
	TestEqual(TEXT("Dialogue wins over Menu"),
		ULRPlayerUIComponent::ComputeEffectiveInputMode(false, true, true), ELRInputMode::Dialogue);
	TestEqual(TEXT("Transition wins over everything"),
		ULRPlayerUIComponent::ComputeEffectiveInputMode(true, true, true), ELRInputMode::Transition);

	// 组件实例：启用/关闭高层后恢复仍然有效的下层，而不是无条件回 Gameplay。
	ULRPlayerUIComponent* ui = NewObject<ULRPlayerUIComponent>();
	TestEqual(TEXT("Component starts at Gameplay"), ui->GetComputedInputMode(), ELRInputMode::Gameplay);
	ui->SetMenuLayer(true);
	TestEqual(TEXT("Menu layer becomes active"), ui->GetComputedInputMode(), ELRInputMode::Menu);
	ui->SetDialogueLayer(true);
	TestEqual(TEXT("Dialogue layer wins over Menu"), ui->GetComputedInputMode(), ELRInputMode::Dialogue);
	ui->SetTransitionLayer(true);
	TestEqual(TEXT("Transition layer wins over Dialogue"), ui->GetComputedInputMode(), ELRInputMode::Transition);
	ui->SetTransitionLayer(false);
	TestEqual(TEXT("Closing Transition restores Dialogue"), ui->GetComputedInputMode(), ELRInputMode::Dialogue);
	ui->SetDialogueLayer(false);
	TestEqual(TEXT("Closing Dialogue restores Menu"), ui->GetComputedInputMode(), ELRInputMode::Menu);
	ui->SetMenuLayer(false);
	TestEqual(TEXT("Closing Menu restores Gameplay"), ui->GetComputedInputMode(), ELRInputMode::Gameplay);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRMenuControllerDirtyTest, "LostRunic.UI.MenuControllerRebuildsOnlyWhileOpen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRMenuControllerDirtyTest::RunTest(const FString& parameters)
{
	ULRItemDefinition* item = MakeItem(TEXT("Test.Key"));
	item->MaxStackSize = 99;
	ULRInventoryComponent* inventory = NewObject<ULRInventoryComponent>();
	inventory->InitializeDefinitions({ item });
	inventory->AddItem(item->ItemId, 1);

	ULRMenuWidgetController* controller = NewObject<ULRMenuWidgetController>();
	ULRTestSnapshotObserver* observer = NewObject<ULRTestSnapshotObserver>();
	controller->Initialize(inventory, NewObject<ULRGameContentSet>(), nullptr);
	controller->OnSnapshotChanged.AddDynamic(observer, &ULRTestSnapshotObserver::HandleSnapshotChanged);

	// 菜单关闭期间收到领域事件只标记 dirty，不广播。
	inventory->AddItem(item->ItemId, 1);
	TestEqual(TEXT("Closed menu does not broadcast snapshot"), observer->BroadcastCount, 0);

	// 打开菜单立即重建并广播一次。
	TestTrue(TEXT("OpenScreen succeeds"), controller->OpenScreen(ELRScreenType::Journal));
	TestEqual(TEXT("Opening rebuilds and broadcasts once"), observer->BroadcastCount, 1);
	TestEqual(TEXT("Cached snapshot reflects opened menu"), observer->LastSnapshot.Items.Num(), 1);
	TestTrue(TEXT("Open snapshot is valid"), observer->LastSnapshot.bIsValid);

	// 菜单可见期间收到领域事件立即重建并广播。
	inventory->AddItem(item->ItemId, 1);
	TestEqual(TEXT("Visible menu rebuilds on domain events"), observer->BroadcastCount, 2);
	TestEqual(TEXT("Rebuilt snapshot carries the new count"), observer->LastSnapshot.Items[0].Quantity, 3);

	// 关闭后事件只标记 dirty。
	controller->CloseScreen();
	inventory->AddItem(item->ItemId, 1);
	TestEqual(TEXT("Closed menu stops broadcasting"), observer->BroadcastCount, 2);

	// 重新打开时带着 dirty 立即重建。
	TestTrue(TEXT("Reopening succeeds"), controller->OpenScreen(ELRScreenType::Inventory));
	TestEqual(TEXT("Reopening broadcasts the dirty snapshot"), observer->BroadcastCount, 3);
	TestEqual(TEXT("Dirty snapshot carries the new count"), observer->LastSnapshot.Items[0].Quantity, 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRMenuScreenClassTest, "LostRunic.UI.MenuScreenClassDerivesFromInventoryScreen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRMenuScreenClassTest::RunTest(const FString& parameters)
{
	// MenuScreenClass 的权威配置在 HUD 蓝图默认值（BP_LRHUD）；类路径仅用于测试定位项目 HUD 蓝图，非运行时逻辑路径。
	const UClass* hudClass = LoadClass<ALRHUD>(nullptr, TEXT("/Game/LostRunic/UI/BP_LRHUD.BP_LRHUD_C"));
	TestNotNull(TEXT("Project HUD Blueprint loads"), hudClass);
	if (!hudClass)
	{
		return true;
	}
	const ALRHUD* hudDefault = hudClass->GetDefaultObject<ALRHUD>();
	const TSubclassOf<ULRScreenWidget> menuScreenClass = hudDefault ? hudDefault->GetMenuScreenClass() : nullptr;
	TestNotNull(TEXT("HUD MenuScreenClass is configured"), menuScreenClass.Get());
	if (menuScreenClass)
	{
		TestTrue(TEXT("MenuScreenClass derives from ULRInventoryScreenWidget"),
			menuScreenClass->IsChildOf(ULRInventoryScreenWidget::StaticClass()));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRInventoryInputMappingTest, "LostRunic.Input.InventoryOpenMappings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRInventoryInputMappingTest::RunTest(const FString& parameters)
{
	const ULRInputConfig* inputConfig = GetDefault<ULRProjectSettings>()->InputConfig.LoadSynchronous();
	TestNotNull(TEXT("Project InputConfig loads"), inputConfig);
	if (!inputConfig)
	{
		return true;
	}

	const UInputMappingContext* gameplayContext = inputConfig->GameplayContext;
	const UInputMappingContext* menuContext = inputConfig->MenuContext;
	TestNotNull(TEXT("Gameplay context exists"), gameplayContext);
	TestNotNull(TEXT("Menu context exists"), menuContext);
	if (!gameplayContext || !menuContext)
	{
		return true;
	}

	// Gameplay 中 I/DPad-Up 映射到 OpenInventoryAction，Tab 不再映射该动作。
	bool bHasKeyI = false;
	bool bHasKeyDPadUp = false;
	bool bHasKeyTab = false;
	for (const FEnhancedActionKeyMapping& mapping : gameplayContext->GetMappings())
	{
		if (mapping.Action != inputConfig->OpenInventoryAction)
		{
			continue;
		}
		const FString keyName = mapping.Key.ToString();
		bHasKeyI |= keyName == TEXT("I");
		bHasKeyDPadUp |= keyName == TEXT("Gamepad_DPad_Up");
		bHasKeyTab |= keyName == TEXT("Tab");
	}
	TestTrue(TEXT("I opens inventory in Gameplay"), bHasKeyI);
	TestTrue(TEXT("DPad-Up opens inventory in Gameplay"), bHasKeyDPadUp);
	TestFalse(TEXT("Tab is no longer mapped to opening inventory"), bHasKeyTab);

	// Menu 中 Tab 映射到 NextTabAction。
	bool bMenuTabToNextTab = false;
	for (const FEnhancedActionKeyMapping& mapping : menuContext->GetMappings())
	{
		if (mapping.Action == inputConfig->NextTabAction && mapping.Key.ToString() == TEXT("Tab"))
		{
			bMenuTabToNextTab = true;
			break;
		}
	}
	TestTrue(TEXT("Tab cycles pages via NextTabAction in Menu"), bMenuTabToNextTab);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRContentCapacityValidationTest, "LostRunic.Content.EditorValidationRejectsOverCapacity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRContentCapacityValidationTest::RunTest(const FString& parameters)
{
	// 笔记与收藏品共享命名容量契约：规则级断言（DataTable 行结构无法经公开 API 在测试中构造）。
	TestTrue(TEXT("Twelve reading rows are within limits"),
		ULRGameContentSet::IsReadingCapacityWithinLimits(12));
	TestFalse(TEXT("Thirteen reading rows exceed the note limit"),
		ULRGameContentSet::IsReadingCapacityWithinLimits(13));
	TestTrue(TEXT("Twelve collectibles are within limits"),
		ULRGameContentSet::IsCollectibleCapacityWithinLimits(12));
	TestFalse(TEXT("Thirteen collectibles exceed the collectible limit"),
		ULRGameContentSet::IsCollectibleCapacityWithinLimits(13));

	// 13 件收藏品定义在任何合法表配置下都不可能通过 Validate（容量契约在 ID 校验后仍会拒绝）。
	ULRGameContentSet* contentSet = NewObject<ULRGameContentSet>();
	FString error;
	contentSet->Collectibles = {
		MakeCollectible(TEXT("Col.01"), 0), MakeCollectible(TEXT("Col.02"), 1), MakeCollectible(TEXT("Col.03"), 2),
		MakeCollectible(TEXT("Col.04"), 3), MakeCollectible(TEXT("Col.05"), 4), MakeCollectible(TEXT("Col.06"), 5),
		MakeCollectible(TEXT("Col.07"), 6), MakeCollectible(TEXT("Col.08"), 7), MakeCollectible(TEXT("Col.09"), 8),
		MakeCollectible(TEXT("Col.10"), 9), MakeCollectible(TEXT("Col.11"), 10), MakeCollectible(TEXT("Col.12"), 11),
		MakeCollectible(TEXT("Col.13"), 12)
	};
	TestFalse(TEXT("Thirteen collectibles never pass content validation"), contentSet->Validate(error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRSaveUISnapshotRulesTest, "LostRunic.UI.SaveSnapshotRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRSaveUISnapshotRulesTest::RunTest(const FString& parameters)
{
	FLRSaveSlotMetadata manualTwo;
	manualTwo.SlotId.Type = ELRSaveSlotType::Manual;
	manualTwo.SlotId.Guid = FGuid::NewGuid();
	manualTwo.DisplayIndex = 2;
	manualTwo.Health = ELRSaveSlotHealth::Healthy;

	FLRSaveSlotMetadata automatic;
	automatic.SlotId.Type = ELRSaveSlotType::Auto;
	automatic.SlotId.Guid = LRSaveV2Ids::AutoSlotGuid;
	automatic.DisplayIndex = 0;
	automatic.Health = ELRSaveSlotHealth::Healthy;

	FLRSaveSlotMetadata manualOne = manualTwo;
	manualOne.SlotId.Guid = FGuid::NewGuid();
	manualOne.DisplayIndex = 1;
	manualOne.Health = ELRSaveSlotHealth::CorruptPayload;

	const FLRSaveUISnapshot save = ULRSaveWidgetController::BuildSnapshot(
		{manualTwo, automatic, manualOne}, ELRSaveSelectionMode::Save, ELRSaveUIState::Idle,
		true, 2, nullptr);
	TestEqual(TEXT("Automatic slot sorts first"), save.Slots[0].SlotId.Type, ELRSaveSlotType::Auto);
	TestEqual(TEXT("Manual slots sort by display index"), save.Slots[1].DisplayIndex, 1);
	TestEqual(TEXT("Second manual slot follows"), save.Slots[2].DisplayIndex, 2);
	TestFalse(TEXT("Automatic slot cannot be overwritten"), save.Slots[0].bCanOverwrite);
	TestFalse(TEXT("Automatic slot cannot be deleted"), save.Slots[0].bCanDelete);
	TestFalse(TEXT("Corrupt slot cannot be loaded"), save.Slots[1].bCanLoad);
	TestFalse(TEXT("Capacity prevents creation"), save.bCanCreateManualSlot);
	TestEqual(TEXT("Create target uses the next manual display index"), save.CreateDisplayIndex, 3);

	const FLRSaveUISnapshot unpaused = ULRSaveWidgetController::BuildSnapshot(
		{automatic, manualOne}, ELRSaveSelectionMode::Save, ELRSaveUIState::Idle,
		false, 3, nullptr);
	TestFalse(TEXT("Unpaused world prevents creation"), unpaused.bCanCreateManualSlot);
	TestFalse(TEXT("Unpaused world prevents overwrite"), unpaused.Slots[1].bCanOverwrite);

	const FLRSaveUISnapshot load = ULRSaveWidgetController::BuildSnapshot(
		{automatic, manualOne}, ELRSaveSelectionMode::Load, ELRSaveUIState::Idle,
		true, 3, nullptr);
	TestFalse(TEXT("Load mode never offers create"), load.bCanCreateManualSlot);
	TestTrue(TEXT("Healthy automatic slot can load"), load.Slots[0].bCanLoad);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRSaveFocusTargetRulesTest, "LostRunic.UI.SaveFocusTargetRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRSaveFocusTargetRulesTest::RunTest(const FString& parameters)
{
	const FLRSaveFocusTarget root = FLRSaveFocusTarget::MakeRoot();
	TestTrue(TEXT("Root focus target is valid"), root.IsValid());
	const FLRSaveFocusTarget create = FLRSaveFocusTarget::MakeCreate(2);
	TestTrue(TEXT("Create focus target carries its display index"), create.IsValid() && create.CreateDisplayIndex == 2);
	FLRSaveSlotId slotId;
	slotId.Type = ELRSaveSlotType::Manual;
	slotId.Guid = FGuid::NewGuid();
	const FLRSaveFocusTarget existing = FLRSaveFocusTarget::MakeExisting(slotId);
	TestTrue(TEXT("Existing-slot focus target carries its stable slot id"), existing.IsValid() && existing.SlotId == slotId);
	TestFalse(TEXT("Zero-index create target is invalid"), FLRSaveFocusTarget::MakeCreate(0).IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRSaveFormattingRulesTest, "LostRunic.UI.SaveFormattingRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRSaveFormattingRulesTest::RunTest(const FString& parameters)
{
	TestEqual(TEXT("Play time keeps hours beyond one day"),
		LRSaveFormatting::FormatPlayTime(90061.9).ToString(), FString(TEXT("25:01:01")));
	TestEqual(TEXT("Negative play time clamps to zero"),
		LRSaveFormatting::FormatPlayTime(-1.0).ToString(), FString(TEXT("00:00:00")));
	const FDateTime utc(2024, 1, 2, 3, 4, 5);
	const FText expected = FText::AsDateTime(utc + FTimespan::FromHours(8), EDateTimeStyle::Short,
		EDateTimeStyle::Short, FText::GetInvariantTimeZone());
	TestEqual(TEXT("Saved-at formatting applies the supplied local offset"),
		LRSaveFormatting::FormatSavedAtWithOffset(utc, FTimespan::FromHours(8)), expected);
	return true;
}

#endif
