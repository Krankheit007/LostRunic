// Copyright LostRunic. All Rights Reserved.
#include "Misc/AutomationTest.h"

#include "Engine/GameInstance.h"
#include "Narrative/LRDialogueScriptRegistry.h"
#include "Narrative/LRStoryStateSubsystem.h"
#include "SUDSScript.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRDialogueScriptRegistryInvariant,
	"LostRunic.Dialogue.RegistryScriptIdInvariant",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRDialogueScriptRegistryInvariant::RunTest(const FString& Parameters)
{
	ULRDialogueScriptRegistry* Registry = NewObject<ULRDialogueScriptRegistry>();
	USUDSScript* Script = NewObject<USUDSScript>();
	FLRDialogueScriptDefinition& First = Registry->Scripts.AddDefaulted_GetRef();
	First.ScriptId = TEXT("Home.Butler.Introduction");
	First.Script = Script;
	FLRDialogueScriptDefinition& Duplicate = Registry->Scripts.AddDefaulted_GetRef();
	Duplicate.ScriptId = TEXT("Home.Butler.Introduction.Alias");
	Duplicate.Script = Script;
	FString Error;
	TestFalse(TEXT("One script cannot have two domain IDs"), Registry->Validate(Error));
	Registry->Scripts.RemoveAt(1);
	TestTrue(TEXT("One valid domain ID resolves"), Registry->Validate(Error));
	TObjectPtr<USUDSScript> Resolved;
	TestTrue(TEXT("Registry resolves the matching script"), Registry->Resolve(First.ScriptId, Resolved, Error));
	TestTrue(TEXT("Resolved pointer matches the registered script"), Resolved == Script);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRStoryStateFlagPersistence,
	"LostRunic.Dialogue.StoryFlagReadWrite",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRStoryStateFlagPersistence::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
	ULRStoryStateSubsystem* StoryState = NewObject<ULRStoryStateSubsystem>(GameInstance);
	const FGameplayTag CompletionTag = FGameplayTag::RequestGameplayTag(
		FName(TEXT("Story.Dialogue.Butler.IntroductionCompleted")), false);
	TestTrue(TEXT("Completion tag is registered"), CompletionTag.IsValid());
	TestTrue(TEXT("First flag write reports a new flag"), StoryState->AddStoryFlag(CompletionTag));
	TestFalse(TEXT("Repeated flag write is idempotent"), StoryState->AddStoryFlag(CompletionTag));
	TestTrue(TEXT("Flag can be queried after write"), StoryState->HasStoryFlag(CompletionTag));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRStoryStateReplaceAndDeltaPersistence,
	"LostRunic.Dialogue.StoryStateReplaceAndApplyDelta",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRStoryStateReplaceAndDeltaPersistence::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
	ULRStoryStateSubsystem* StoryState = NewObject<ULRStoryStateSubsystem>(GameInstance);
	const FGameplayTag FirstFlag = FGameplayTag::RequestGameplayTag(FName(TEXT("Story.Dialogue.Home.First")), false);
	const FGameplayTag SecondFlag = FGameplayTag::RequestGameplayTag(FName(TEXT("Story.Dialogue.Home.Second")), false);
	TestTrue(TEXT("First flag is registered"), FirstFlag.IsValid());
	TestTrue(TEXT("Second flag is registered"), SecondFlag.IsValid());

	FLRNarrativePersistentState PersistentState;
	PersistentState.StoryFlags.AddTag(FirstFlag);
	PersistentState.CompletedEventIds.Add(TEXT("Home.Event.First"));
	PersistentState.MemoryEventIds.Add(TEXT("Memory.Entry"));
	TestTrue(TEXT("Replace accepts a valid full snapshot"), StoryState->ReplacePersistentState(PersistentState));

	FLRNarrativePersistentDelta Delta;
	Delta.AddedStoryFlags.AddTag(SecondFlag);
	Delta.AddedCompletedEventIds.Add(TEXT("Home.Event.Second"));
	Delta.AddedMemoryEventIds.Add(TEXT("Memory.Return"));
	TestTrue(TEXT("Delta applies once"), StoryState->ApplyPersistentDelta(Delta));
	TestTrue(TEXT("Delta applies idempotently"), StoryState->ApplyPersistentDelta(Delta));

	FLRNarrativePersistentState CapturedState;
	StoryState->CapturePersistentState(CapturedState);
	TestTrue(TEXT("Replace keeps the initial story flag"), CapturedState.StoryFlags.HasTagExact(FirstFlag));
	TestTrue(TEXT("ApplyDelta appends the second story flag"), CapturedState.StoryFlags.HasTagExact(SecondFlag));
	TestEqual(TEXT("ApplyDelta stays append-only for completed events"), CapturedState.CompletedEventIds.Num(), 2);
	TestEqual(TEXT("ApplyDelta stays append-only for memory events"), CapturedState.MemoryEventIds.Num(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRStoryStateCommitBroadcastOrdering,
	"LostRunic.Dialogue.StoryEventCommitMutatesBeforeBroadcast",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRStoryStateCommitBroadcastOrdering::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
	ULRStoryStateSubsystem* StoryState = NewObject<ULRStoryStateSubsystem>(GameInstance);
	bool bObservedCommittedState = false;
	StoryState->OnStoryEventCommittedNative.AddLambda(
		[&bObservedCommittedState, StoryState](const FLRStoryEventCommit& EventCommit)
		{
			bObservedCommittedState = StoryState->IsEventCompleted(EventCommit.EventId);
		});

	FLRStoryEventCommit EventCommit;
	EventCommit.EventId = TEXT("Home.Event.BroadcastOrder");
	EventCommit.SavePolicy = ELRSavePolicy::Critical;
	TestTrue(TEXT("First commit succeeds"), StoryState->CommitEvent(EventCommit));
	TestTrue(TEXT("Broadcast observes committed state"), bObservedCommittedState);
	TestFalse(TEXT("Duplicate completion is rejected"), StoryState->CommitEvent(EventCommit));
	return true;
}

#endif
