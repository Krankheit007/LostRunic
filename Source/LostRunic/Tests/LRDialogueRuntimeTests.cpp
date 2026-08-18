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

#endif
