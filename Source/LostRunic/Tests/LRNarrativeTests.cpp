/**
 * @file LRNarrativeTests.cpp
 * @brief 提供 LostRunic Runtime 自动化测试，覆盖调优边界、状态矩阵、交互筛选、物品双入口、守卫警戒、叙事分支和存档事务顺序。仅在 WITH_DEV_AUTOMATION_TESTS 下编译。
 *
 * 关联文件：Tests 目录内调用该公共契约的实现文件；所属领域：Tests。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/LRGameplayTags.h"
#include "Data/LRContentRows.h"
#include "Data/LRCollectibleDefinition.h"
#include "Data/LRGameContentSet.h"
#include "Data/LRGuardDefinition.h"
#include "Data/LRItemDefinition.h"
#include "Data/LRLevelEventDefinition.h"
#include "Data/LRUITuning.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Internationalization/StringTable.h"
#include "Narrative/LRDialogueSubsystem.h"
#include "Narrative/LRNarrativeRules.h"
#include "UI/LRDialogueWidgetController.h"
#include "UI/LRMenuWidgetController.h"

namespace
{
	/**
	 * @brief 根据当前领域状态构建 Make Content Set 所需的数据，不把临时对象作为长期存档标识。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	ULRGameContentSet* MakeContentSet()
	{
		ULRGameContentSet* contentSet = NewObject<ULRGameContentSet>();
		contentSet->ReadingTable = NewObject<UDataTable>(contentSet);
		contentSet->ReadingTable->RowStruct = FLRReadingRow::StaticStruct();
		contentSet->UIStringTable = NewObject<UStringTable>(contentSet);
		// 地图注册校验：NewGameMapId 必须解析到已注册地图（内容集校验的一部分）。
		FLRMapRegistration mapRegistration;
		mapRegistration.MapId = TEXT("Map.Test");
		mapRegistration.DisplayNameTextKey = TEXT("Map.Test.DisplayName");
		mapRegistration.DefaultStartAnchorId = TEXT("Map.Test.Start");
		mapRegistration.World = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/LostRunic/Levels/PIE_Test/L_PIE_Test.L_PIE_Test")));
		contentSet->Maps.Add(mapRegistration);
		contentSet->NewGameMapId = mapRegistration.MapId;
		contentSet->MainMenuMapId = mapRegistration.MapId;
		return contentSet;
	}

	/**
	 * @brief 根据当前领域状态构建 Make Dialogue Subsystem 所需的数据，不把临时对象作为长期存档标识。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	ULRDialogueSubsystem* MakeDialogueSubsystem()
	{
		UGameInstance* gameInstance = NewObject<UGameInstance>();
		return NewObject<ULRDialogueSubsystem>(gameInstance);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRNarrativeConditionsTest, "LostRunic.Narrative.ConditionsFilterChoices",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRNarrativeConditionsTest::RunTest(const FString& parameters)
{
	FLRDialogueOption available;
	available.OptionId = TEXT("Available");
	available.NextRowId = TEXT("Next");
	available.RequiredTags.AddTag(LRGameplayTags::NarrativeEventCompleted);
	FLRDialogueOption blocked;
	blocked.OptionId = TEXT("Blocked");
	blocked.BlockedTags.AddTag(LRGameplayTags::NarrativeEventCompleted);

	FGameplayTagContainer contextTags;
	contextTags.AddTag(LRGameplayTags::NarrativeEventCompleted);
	TArray<FLRNarrativeChoice> choices;
	LRNarrativeRules::BuildAvailableChoices({ available, blocked }, contextTags, choices);
	TestEqual(TEXT("Only the legal choice is exposed"), choices.Num(), 1);
	TestEqual(TEXT("Available branch retains stable ID"), choices[0].NextContentId, FName(TEXT("Next")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRContentRegistryTest, "LostRunic.Content.RegistryValidatesAndFindsDefinitions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRContentRegistryTest::RunTest(const FString& parameters)
{
	ULRGameContentSet* contentSet = MakeContentSet();
	ULRItemDefinition* item = NewObject<ULRItemDefinition>(contentSet);
	item->ItemId = TEXT("Key.Home");
	ULRCollectibleDefinition* collectible = NewObject<ULRCollectibleDefinition>(contentSet);
	collectible->CollectibleId = TEXT("Collectible.Doll");
	ULRGuardDefinition* guard = NewObject<ULRGuardDefinition>(contentSet);
	guard->GuardId = TEXT("Guard.Home");
	contentSet->Items.Add(item);
	contentSet->Collectibles.Add(collectible);
	contentSet->Guards.Add(guard);

	FString error;
	TestTrue(TEXT("Content definitions validate"), contentSet->Validate(error));
	contentSet->Maps[0].DisplayNameTextKey = NAME_None;
	error.Reset();
	TestFalse(TEXT("Map registrations require a localized display-name TextKey"), contentSet->Validate(error));
	contentSet->Maps[0].DisplayNameTextKey = TEXT("Map.Test.DisplayName");
	TestTrue(TEXT("Item definition is found by stable ID"), contentSet->FindItemDefinition(TEXT("Key.Home")) == item);
	TestTrue(TEXT("Collectible definition is found by stable ID"),
		contentSet->FindCollectibleDefinition(TEXT("Collectible.Doll")) == collectible);
	TestTrue(TEXT("Guard definition is found by stable ID"), contentSet->FindGuardDefinition(TEXT("Guard.Home")) == guard);

	ULRItemDefinition* duplicate = NewObject<ULRItemDefinition>(contentSet);
	duplicate->ItemId = item->ItemId;
	contentSet->Items.Add(duplicate);
	error.Reset();
	TestFalse(TEXT("Duplicate item IDs are rejected"), contentSet->Validate(error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRNarrativeBranchingTest, "LostRunic.Narrative.BranchingAndOneShotEvents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRNarrativeBranchingTest::RunTest(const FString& parameters)
{
	ULRGameContentSet* contentSet = MakeContentSet();
	ULRLevelEventDefinition* eventDefinition = NewObject<ULRLevelEventDefinition>(contentSet);
	eventDefinition->EventId = TEXT("Home.Dorothy.Spoken");
	contentSet->LevelEvents.Add(eventDefinition);

	ULRDialogueSubsystem* subsystem = MakeDialogueSubsystem();
	subsystem->InitializeContent(contentSet);
	TestTrue(TEXT("Story event commits independently of dialogue presentation"), subsystem->TryCompleteEvent(eventDefinition->EventId).bSuccess);
	TestTrue(TEXT("One-shot event is recorded"), subsystem->IsEventCompleted(eventDefinition->EventId));
	TestTrue(TEXT("Repeated one-shot event is rejected"), subsystem->TryCompleteEvent(eventDefinition->EventId).FailureReason
		== LRGameplayTags::NarrativeRejectAlreadyCompleted);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRNarrativeReadingTest, "LostRunic.Narrative.ReadingUsesSharedSessionFlow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRNarrativeReadingTest::RunTest(const FString& parameters)
{
	ULRGameContentSet* contentSet = MakeContentSet();
	FLRReadingRow reading;
	reading.ReadingId = TEXT("Home_Note_Mother");
	reading.Title = FText::FromString(TEXT("Note"));
	reading.Body = FText::FromString(TEXT("A short note."));
	contentSet->ReadingTable->AddRow(reading.ReadingId, reading);

	ULRDialogueSubsystem* subsystem = MakeDialogueSubsystem();
	subsystem->InitializeContent(contentSet);
	TestTrue(TEXT("Reading starts through the narrative subsystem"), subsystem->StartReading(reading.ReadingId).bSuccess);
	TestEqual(TEXT("Reading uses its own presentation type"), subsystem->GetCurrentPage().SessionType, ELRNarrativeSessionType::Reading);
	TestTrue(TEXT("Reading advances through the shared completion transaction"), subsystem->Advance().bSuccess);
	TestFalse(TEXT("Reading session closes"), subsystem->HasActiveSession());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRNarrativeTypewriterTest, "LostRunic.UI.TypewriterRevealsBeforeAdvancing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRNarrativeTypewriterTest::RunTest(const FString& parameters)
{
	ULRGameContentSet* contentSet = MakeContentSet();
	FLRReadingRow reading;
	reading.ReadingId = TEXT("TypewriterNote");
	reading.Title = FText::FromString(TEXT("Note"));
	reading.Body = FText::FromString(TEXT("ABCDE"));
	contentSet->ReadingTable->AddRow(reading.ReadingId, reading);
	ULRDialogueSubsystem* subsystem = MakeDialogueSubsystem();
	subsystem->InitializeContent(contentSet);
	ULRUITuning* tuning = NewObject<ULRUITuning>();
	tuning->TypewriterCharactersPerSecond = 2.0f;
	ULRDialogueWidgetController* controller = NewObject<ULRDialogueWidgetController>();
	controller->Initialize(subsystem, tuning, nullptr);

	TestTrue(TEXT("Reading starts"), subsystem->StartReading(reading.ReadingId).bSuccess);
	controller->AdvanceTypewriterForTest(1.0f);
	TestEqual(TEXT("Typewriter uses tuned speed"), controller->GetPresentation().DisplayedText.ToString(), FString(TEXT("AB")));
	TestEqual(TEXT("First confirm reveals full text"), controller->HandleConfirm().Action, ELRNarrativeAction::RevealCurrentText);
	TestEqual(TEXT("Full text is displayed"), controller->GetPresentation().DisplayedText.ToString(), FString(TEXT("ABCDE")));
	TestEqual(TEXT("Second confirm completes the reading session"), controller->HandleConfirm().Action, ELRNarrativeAction::Completed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRMenuScreenControllerTest, "LostRunic.UI.MenuControllerAcceptsOnlyMenuScreens",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRMenuScreenControllerTest::RunTest(const FString& parameters)
{
	ULRMenuWidgetController* controller = NewObject<ULRMenuWidgetController>();
	TestFalse(TEXT("HUD is not a menu screen"), controller->OpenScreen(ELRScreenType::HUD));
	TestTrue(TEXT("Pause is a legal menu screen"), controller->OpenScreen(ELRScreenType::Pause));
	TestEqual(TEXT("Opened screen is retained"), controller->GetOpenScreen(), ELRScreenType::Pause);
	controller->CloseScreen();
	TestEqual(TEXT("Close resets menu state"), controller->GetOpenScreen(), ELRScreenType::None);
	return true;
}

#endif
