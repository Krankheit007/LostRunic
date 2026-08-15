/**
 * @file LRGuardStateTreeTests.cpp
 * @brief 验证守卫 StateTree 原生节点继承、资产契约、Root 重选、持续 Running 和 Investigate 同状态重定位。
 *
 * 关联文件：AI/LRGuardStateTreeNodes.h、AI/LRGuardAIController.h；所属领域：Tests。
 */
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AI/LRAlertComponent.h"
#include "AI/LRGuardAIController.h"
#include "AI/LRGuardCharacter.h"
#include "AI/LRGuardStateTreeNodes.h"
#include "AI/LRNPCStateTreeNodes.h"
#include "Core/LRGameplayTags.h"
#include "Data/LRGuardDefinition.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/StateTreeAIComponentSchema.h"
#include "Components/StateTreeAIComponent.h"
#include "StateTree.h"
#include "StateTreeConditionBase.h"
#include "StateTreeTaskBase.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRStateTreeNodeSchemaCompatibilityTest, "LostRunic.AI.StateTreeNodeSchemaCompatibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLRStateTreeNodeSchemaCompatibilityTest::RunTest(const FString& parameters)
{
	TestTrue(TEXT("Guard condition uses the common StateTree condition base"),
		FLRGuardStateCondition::StaticStruct()->IsChildOf(FStateTreeConditionCommonBase::StaticStruct()));
	TestTrue(TEXT("NPC condition uses the common StateTree condition base"),
		FLRNPCStateCondition::StaticStruct()->IsChildOf(FStateTreeConditionCommonBase::StaticStruct()));
	TestTrue(TEXT("Guard behavior task uses the common StateTree task base"),
		FLRGuardBehaviorTask::StaticStruct()->IsChildOf(FStateTreeTaskCommonBase::StaticStruct()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRGuardStateTreeAssetContractTest, "LostRunic.AI.GuardStateTreeAssetContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLRGuardStateTreeAssetContractTest::RunTest(const FString& parameters)
{
	UStateTree* stateTree = LoadObject<UStateTree>(nullptr, TEXT("/Game/LostRunic/Blueprints/Guard/ST_Guard.ST_Guard"));
	if (!TestNotNull(TEXT("Guard StateTree asset loads"), stateTree))
	{
		return false;
	}

	TestTrue(TEXT("Guard StateTree is ready to run"), stateTree->IsReadyToRun());
	const UStateTreeAIComponentSchema* schema = Cast<UStateTreeAIComponentSchema>(stateTree->GetSchema());
	if (!TestNotNull(TEXT("Guard StateTree uses the AI component schema"), schema))
	{
		return false;
	}

	TestEqual(TEXT("Guard StateTree context actor is the guard character"), schema->GetContextActorClass(), ALRGuardCharacter::StaticClass());
	const TConstArrayView<FStateTreeExternalDataDesc> contextData = stateTree->GetContextDataDescs();
	TestEqual(TEXT("Guard StateTree exposes actor and controller context"), contextData.Num(), 2);
	if (contextData.Num() == 2)
	{
		TestTrue(TEXT("Guard StateTree controller context is the guard controller"), contextData[1].Struct.Get() == ALRGuardAIController::StaticClass());
	}

	const TConstArrayView<FCompactStateTreeState> states = stateTree->GetStates();
	TestEqual(TEXT("Guard StateTree has one root and six behavior states"), states.Num(), 7);
	if (states.Num() != 7)
	{
		return false;
	}

	TestEqual(TEXT("Root state name"), states[0].Name, FName(TEXT("Root")));
	TestEqual(TEXT("Root state type"), states[0].Type, EStateTreeStateType::Group);
	TestEqual(TEXT("Root selects children in order"), states[0].SelectionBehavior, EStateTreeStateSelectionBehavior::TrySelectChildrenInOrder);
	TestEqual(TEXT("Root has no tasks"), states[0].TasksNum, uint8(0));

	const TArray<ELRGuardBehaviorState> expectedBehaviors = {
		ELRGuardBehaviorState::IdlePatrol,
		ELRGuardBehaviorState::Suspicious,
		ELRGuardBehaviorState::Investigate,
		ELRGuardBehaviorState::Search,
		ELRGuardBehaviorState::Chase,
		ELRGuardBehaviorState::Stunned
	};
	const TArray<FName> expectedNames = {
		FName(TEXT("IdlePatrol")), FName(TEXT("Suspicious")), FName(TEXT("Investigate")),
		FName(TEXT("Search")), FName(TEXT("Chase")), FName(TEXT("Stunned"))
	};

	for (int32 stateIndex = 1; stateIndex < states.Num(); ++stateIndex)
	{
		const FCompactStateTreeState& state = states[stateIndex];
		const int32 behaviorIndex = stateIndex - 1;
		TestEqual(TEXT("Behavior state name"), state.Name, expectedNames[behaviorIndex]);
		TestEqual(TEXT("Behavior state has one enter condition"), state.EnterConditionsNum, uint8(1));
		TestEqual(TEXT("Behavior state has one task"), state.TasksNum, uint8(1));
		TestEqual(TEXT("Behavior state has one BehaviorChanged transition"), state.TransitionsNum, uint8(1));

		const FConstStructView conditionNode = stateTree->GetNode(state.EnterConditionsBegin);
		TestTrue(TEXT("Behavior state condition type"), conditionNode.GetScriptStruct() == FLRGuardStateCondition::StaticStruct());
		if (conditionNode.GetScriptStruct() == FLRGuardStateCondition::StaticStruct())
		{
			TestEqual(TEXT("Behavior state condition enum"), conditionNode.Get<const FLRGuardStateCondition>().ExpectedBehavior, expectedBehaviors[behaviorIndex]);
		}

		const FConstStructView taskNode = stateTree->GetNode(state.TasksBegin);
		TestTrue(TEXT("Behavior state task type"), taskNode.GetScriptStruct() == FLRGuardBehaviorTask::StaticStruct());
		if (taskNode.GetScriptStruct() == FLRGuardBehaviorTask::StaticStruct())
		{
			const FLRGuardBehaviorTask& task = taskNode.Get<const FLRGuardBehaviorTask>();
			TestEqual(TEXT("Behavior state task enum"), task.Behavior, expectedBehaviors[behaviorIndex]);
			TestFalse(TEXT("Behavior task does not tick"), task.bShouldCallTick);
		}

		const FCompactStateTransition* transition = stateTree->GetTransitionFromIndex(FStateTreeIndex16(state.TransitionsBegin));
		if (!TestNotNull(TEXT("BehaviorChanged transition exists"), transition))
		{
			continue;
		}
		TestEqual(TEXT("BehaviorChanged transition trigger"), transition->Trigger, EStateTreeTransitionTrigger::OnEvent);
		TestTrue(TEXT("BehaviorChanged transition event"), transition->RequiredEvent.Tag == LRGameplayTags::AIEventBehaviorChanged);
		TestEqual(TEXT("BehaviorChanged transition priority"), transition->Priority, EStateTreeTransitionPriority::Normal);
		TestTrue(TEXT("BehaviorChanged transition consumes the event"), transition->bConsumeEventOnSelect);
		TestEqual(TEXT("BehaviorChanged transition targets Root"), transition->State, FStateTreeStateHandle::Root);
		TestEqual(TEXT("BehaviorChanged transition forces root re-selection"), transition->ChangeTypeTargetStateRule, EStateTreeTransitionChangeTypeRules::ForceChanged);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRGuardStateTreePersistentRunningTest, "LostRunic.AI.GuardStateTreePersistentRunning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLRGuardStateTreePersistentRunningTest::RunTest(const FString& parameters)
{
	UStateTree* stateTree = LoadObject<UStateTree>(nullptr, TEXT("/Game/LostRunic/Blueprints/Guard/ST_Guard.ST_Guard"));
	if (!TestNotNull(TEXT("Guard StateTree asset loads"), stateTree))
	{
		return false;
	}

	if (!TestNotNull(TEXT("Engine exists for the isolated StateTree world"), GEngine))
	{
		return false;
	}

	const FName testWorldName = MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass(), TEXT("GuardStateTreePersistentRunningWorld"));
	UWorld* testWorld = UWorld::CreateWorld(EWorldType::Game, false, testWorldName, GetTransientPackage());
	if (!TestNotNull(TEXT("Isolated StateTree world creates"), testWorld))
	{
		return false;
	}
	FWorldContext& worldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	worldContext.SetCurrentWorld(testWorld);

	FActorSpawnParameters spawnParameters;
	spawnParameters.ObjectFlags = RF_Transient;
	ALRGuardCharacter* guard = testWorld->SpawnActor<ALRGuardCharacter>(spawnParameters);
	ALRGuardAIController* controller = testWorld->SpawnActor<ALRGuardAIController>(spawnParameters);
	UStateTreeAIComponent* stateTreeAI = controller ? controller->FindComponentByClass<UStateTreeAIComponent>() : nullptr;
	bool bTestPassed = TestNotNull(TEXT("Isolated guard pawn spawns"), guard)
		&& TestNotNull(TEXT("Isolated guard controller spawns"), controller)
		&& TestNotNull(TEXT("Isolated guard controller creates StateTree component"), stateTreeAI);
	if (bTestPassed)
	{
		ULRGuardDefinition* definition = LoadObject<ULRGuardDefinition>(nullptr, TEXT("/Game/LostRunic/Data/Guard/DA_LRGuardDefinition.DA_LRGuardDefinition"));
		FObjectPropertyBase* definitionProperty = FindFProperty<FObjectPropertyBase>(ALRGuardCharacter::StaticClass(), TEXT("Definition"));
		bTestPassed &= TestNotNull(TEXT("Guard Definition asset loads"), definition);
		bTestPassed &= TestNotNull(TEXT("Guard Definition property is available"), definitionProperty);
		if (definition && definitionProperty)
		{
			definitionProperty->SetObjectPropertyValue_InContainer(guard, definition);
		}
		controller->Possess(guard);

		if (!stateTreeAI->IsRunning())
		{
			stateTreeAI->SetStartLogicAutomatically(false);
			stateTreeAI->SetStateTree(stateTree);
			stateTreeAI->StartLogic();
		}
		bTestPassed &= TestEqual(TEXT("Guard StateTree starts Running"), stateTreeAI->GetStateTreeRunStatus(), EStateTreeRunStatus::Running);

#if WITH_GAMEPLAY_DEBUGGER
		const TArray<FName> activeStates = stateTreeAI->GetActiveStateNames();
		bTestPassed &= TestTrue(TEXT("Guard StateTree enters Root"), activeStates.Contains(FName(TEXT("Root"))));
		bTestPassed &= TestTrue(TEXT("Guard StateTree enters IdlePatrol"), activeStates.Contains(FName(TEXT("IdlePatrol"))));
#endif

		for (int32 updateIndex = 0; updateIndex < 3; ++updateIndex)
		{
			testWorld->Tick(LEVELTICK_All, 0.016f);
			bTestPassed &= TestEqual(TEXT("Guard StateTree remains Running without an event"), stateTreeAI->GetStateTreeRunStatus(), EStateTreeRunStatus::Running);
		}

#if WITH_GAMEPLAY_DEBUGGER
		const TArray<FName> activeStatesAfterUpdates = stateTreeAI->GetActiveStateNames();
		bTestPassed &= TestTrue(TEXT("IdlePatrol remains Active after updates"), activeStatesAfterUpdates.Contains(FName(TEXT("IdlePatrol"))));
#endif

		auto ReselectFromRoot = [&stateTreeAI]()
		{
			stateTreeAI->SendStateTreeEvent(LRGameplayTags::AIEventBehaviorChanged, FConstStructView(), FName());
			for (int32 eventUpdateIndex = 0; eventUpdateIndex < 3; ++eventUpdateIndex)
			{
				stateTreeAI->TickComponent(0.016f, LEVELTICK_All, nullptr);
			}
		};

		ReselectFromRoot();
		bTestPassed &= TestEqual(TEXT("BehaviorChanged leaves the task Running after re-selection"), stateTreeAI->GetStateTreeRunStatus(), EStateTreeRunStatus::Running);

		guard->GetAlertComponent()->ApplyAlertDelta(1, FVector(50.0f, 0.0f, 0.0f), nullptr, LRGameplayTags::NoiseInteraction);
		bTestPassed &= TestEqual(TEXT("Guard controller resolves Suspicious"), controller->GetResolvedBehavior(), ELRGuardBehaviorState::Suspicious);
		ReselectFromRoot();
	#if WITH_GAMEPLAY_DEBUGGER
		bTestPassed &= TestTrue(TEXT("Root re-selection selects Suspicious"), stateTreeAI->GetActiveStateNames().Contains(FName(TEXT("Suspicious"))));
	#endif

		const FVector firstInvestigationLocation(100.0f, 0.0f, 0.0f);
		guard->GetAlertComponent()->ApplyAlertDelta(5, firstInvestigationLocation, nullptr, LRGameplayTags::NoiseInteraction);
		bTestPassed &= TestEqual(TEXT("Guard controller resolves Investigate"), controller->GetResolvedBehavior(), ELRGuardBehaviorState::Investigate);
		ReselectFromRoot();
	#if WITH_GAMEPLAY_DEBUGGER
		bTestPassed &= TestTrue(TEXT("BehaviorChanged selects Investigate"), stateTreeAI->GetActiveStateNames().Contains(FName(TEXT("Investigate"))));
	#endif

		const FVector secondInvestigationLocation(250.0f, 50.0f, 0.0f);
		guard->GetAlertComponent()->ApplyAlertDelta(0, secondInvestigationLocation, nullptr, LRGameplayTags::NoiseInteraction);
		for (int32 eventUpdateIndex = 0; eventUpdateIndex < 3; ++eventUpdateIndex)
		{
			stateTreeAI->TickComponent(0.016f, LEVELTICK_All, nullptr);
		}
		bTestPassed &= TestEqual(TEXT("Investigate updates its disturbance location"), guard->GetAlertComponent()->GetLastDisturbanceLocation(), secondInvestigationLocation);
	#if WITH_GAMEPLAY_DEBUGGER
		bTestPassed &= TestTrue(TEXT("Same-state Investigate update remains Active"), stateTreeAI->GetActiveStateNames().Contains(FName(TEXT("Investigate"))));
	#endif

		guard->GetAlertComponent()->MarkInvestigationReached();
		ReselectFromRoot();
	#if WITH_GAMEPLAY_DEBUGGER
		bTestPassed &= TestTrue(TEXT("A real behavior change reselects Search"), stateTreeAI->GetActiveStateNames().Contains(FName(TEXT("Search"))));
	#endif

		guard->GetAlertComponent()->SetSightTarget(guard, true, FVector(300.0f, 0.0f, 0.0f));
		bTestPassed &= TestEqual(TEXT("Guard controller resolves Chase"), controller->GetResolvedBehavior(), ELRGuardBehaviorState::Chase);
		ReselectFromRoot();
	#if WITH_GAMEPLAY_DEBUGGER
		bTestPassed &= TestTrue(TEXT("Root re-selection selects Chase"), stateTreeAI->GetActiveStateNames().Contains(FName(TEXT("Chase"))));
	#endif

		guard->GetAlertComponent()->ResetAfterSearch();
		bTestPassed &= TestEqual(TEXT("Guard controller resolves IdlePatrol after reset"), controller->GetResolvedBehavior(), ELRGuardBehaviorState::IdlePatrol);
		ReselectFromRoot();
	#if WITH_GAMEPLAY_DEBUGGER
		bTestPassed &= TestTrue(TEXT("Root re-selection returns to IdlePatrol"), stateTreeAI->GetActiveStateNames().Contains(FName(TEXT("IdlePatrol"))));
	#endif

	}

	if (controller)
	{
		controller->UnPossess();
	}
	testWorld->EndPlay(EEndPlayReason::Quit);
	GEngine->DestroyWorldContext(testWorld);
	testWorld->DestroyWorld(false);
	return bTestPassed;
}


#endif
