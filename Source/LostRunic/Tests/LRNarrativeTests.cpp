#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/LRGameplayTags.h"
#include "Data/LRContentRows.h"
#include "Data/LRGameContentSet.h"
#include "Data/LRLevelEventDefinition.h"
#include "Data/LRUITuning.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Narrative/LRDialogueSubsystem.h"
#include "Narrative/LRNarrativeRules.h"
#include "UI/LRDialogueWidgetController.h"
#include "UI/LRMenuWidgetController.h"

namespace
{
	ULRGameContentSet* MakeContentSet()
	{
		ULRGameContentSet* contentSet = NewObject<ULRGameContentSet>();
		contentSet->DialogueTable = NewObject<UDataTable>(contentSet);
		contentSet->DialogueTable->RowStruct = FLRDialogueRow::StaticStruct();
		contentSet->ReadingTable = NewObject<UDataTable>(contentSet);
		contentSet->ReadingTable->RowStruct = FLRReadingRow::StaticStruct();
		return contentSet;
	}

	FLRDialogueRow MakeDialogueRow(const FName rowId, const FString& text, const FName nextRowId = NAME_None)
	{
		FLRDialogueRow row;
		row.DialogueId = rowId;
		row.Text = FText::FromString(text);
		row.NextRowId = nextRowId;
		return row;
	}

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRNarrativeBranchingTest, "LostRunic.Narrative.BranchingAndOneShotEvents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRNarrativeBranchingTest::RunTest(const FString& parameters)
{
	ULRGameContentSet* contentSet = MakeContentSet();
	FLRDialogueRow start = MakeDialogueRow(TEXT("Start"), TEXT("Choose."));
	FLRDialogueOption option;
	option.OptionId = TEXT("Continue");
	option.NextRowId = TEXT("End");
	start.Options.Add(option);
	contentSet->DialogueTable->AddRow(TEXT("Start"), start);
	contentSet->DialogueTable->AddRow(TEXT("End"), MakeDialogueRow(TEXT("End"), TEXT("Finished.")));
	ULRLevelEventDefinition* eventDefinition = NewObject<ULRLevelEventDefinition>(contentSet);
	eventDefinition->EventId = TEXT("Home.Dorothy.Spoken");
	contentSet->LevelEvents.Add(eventDefinition);

	ULRDialogueSubsystem* subsystem = MakeDialogueSubsystem();
	subsystem->InitializeContent(contentSet);
	TestTrue(TEXT("Dialogue starts"), subsystem->StartDialogue(TEXT("Start"), eventDefinition->EventId).bSuccess);
	TestTrue(TEXT("Legal branch advances"), subsystem->SelectChoice(TEXT("Continue")).bSuccess);
	TestEqual(TEXT("Second line became current"), subsystem->GetCurrentPage().ContentId, FName(TEXT("End")));
	TestTrue(TEXT("Final advance completes event"), subsystem->Advance().bSuccess);
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
	contentSet->DialogueTable->AddRow(TEXT("Line"), MakeDialogueRow(TEXT("Line"), TEXT("ABCDE")));
	ULRDialogueSubsystem* subsystem = MakeDialogueSubsystem();
	subsystem->InitializeContent(contentSet);
	ULRUITuning* tuning = NewObject<ULRUITuning>();
	tuning->TypewriterCharactersPerSecond = 2.0f;
	ULRDialogueWidgetController* controller = NewObject<ULRDialogueWidgetController>();
	controller->Initialize(subsystem, tuning, nullptr);

	TestTrue(TEXT("Dialogue starts"), subsystem->StartDialogue(TEXT("Line")).bSuccess);
	controller->AdvanceTypewriterForTest(1.0f);
	TestEqual(TEXT("Typewriter uses tuned speed"), controller->GetPresentation().DisplayedText.ToString(), FString(TEXT("AB")));
	TestEqual(TEXT("First confirm reveals full text"), controller->HandleConfirm().Action, ELRNarrativeAction::RevealCurrentText);
	TestEqual(TEXT("Full text is displayed"), controller->GetPresentation().DisplayedText.ToString(), FString(TEXT("ABCDE")));
	TestEqual(TEXT("Second confirm advances the rule session"), controller->HandleConfirm().Action, ELRNarrativeAction::Completed);
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
