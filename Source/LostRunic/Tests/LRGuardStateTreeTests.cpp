/**
 * @file LRGuardStateTreeTests.cpp
 * @brief 验证五个 Guard StateTree 状态、资产契约和持续 Running。
 */
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AI/LRGuardAIController.h"
#include "AI/LRGuardCharacter.h"
#include "AI/LRGuardStateTreeNodes.h"
#include "AI/LRNPCStateTreeNodes.h"
#include "Core/LRGameplayTags.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/StateTreeAIComponent.h"
#include "Components/StateTreeAIComponentSchema.h"
#include "StateTree.h"
#include "StateTreeConditionBase.h"
#include "StateTreeTaskBase.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRStateTreeNodeSchemaCompatibilityTest, "LostRunic.AI.StateTreeNodeSchemaCompatibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLRStateTreeNodeSchemaCompatibilityTest::RunTest(const FString& parameters)
{
	(void)parameters;
	TestTrue(TEXT("Guard condition uses common StateTree condition base"),
		FLRGuardStateCondition::StaticStruct()->IsChildOf(FStateTreeConditionCommonBase::StaticStruct()));
	TestTrue(TEXT("NPC condition uses common StateTree condition base"),
		FLRNPCStateCondition::StaticStruct()->IsChildOf(FStateTreeConditionCommonBase::StaticStruct()));
	TestTrue(TEXT("Guard behavior task uses common StateTree task base"),
		FLRGuardBehaviorTask::StaticStruct()->IsChildOf(FStateTreeTaskCommonBase::StaticStruct()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRGuardStateTreeAssetContractTest, "LostRunic.AI.GuardStateTreeAssetContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLRGuardStateTreeAssetContractTest::RunTest(const FString& parameters)
{
	(void)parameters;
	UStateTree* stateTree = LoadObject<UStateTree>(nullptr, TEXT("/Game/LostRunic/Blueprints/Guard/ST_Guard.ST_Guard"));
	if (!TestNotNull(TEXT("Guard StateTree asset loads"), stateTree))
	{
		return false;
	}

	TestTrue(TEXT("Guard StateTree is ready to run"), stateTree->IsReadyToRun());
	const UStateTreeAIComponentSchema* schema = Cast<UStateTreeAIComponentSchema>(stateTree->GetSchema());
	if (!TestNotNull(TEXT("Guard StateTree uses AI component schema"), schema))
	{
		return false;
	}
	TestEqual(TEXT("StateTree context actor is Guard character"), schema->GetContextActorClass(), ALRGuardCharacter::StaticClass());
	const TConstArrayView<FStateTreeExternalDataDesc> contextData = stateTree->GetContextDataDescs();
	TestEqual(TEXT("StateTree exposes actor and controller context"), contextData.Num(), 2);
	if (contextData.Num() == 2)
	{
		TestTrue(TEXT("StateTree controller context is Guard controller"),
			contextData[1].Struct.Get() == ALRGuardAIController::StaticClass());
	}

	const TConstArrayView<FCompactStateTreeState> states = stateTree->GetStates();
	TestEqual(TEXT("StateTree has one root and five behavior states"), states.Num(), 6);
	if (states.Num() != 6)
	{
		return false;
	}
	TestEqual(TEXT("Root state name"), states[0].Name, FName(TEXT("Root")));
	TestEqual(TEXT("Root state type"), states[0].Type, EStateTreeStateType::Group);
	TestEqual(TEXT("Root selects children in order"), states[0].SelectionBehavior,
		EStateTreeStateSelectionBehavior::TrySelectChildrenInOrder);

	const TArray<ELRGuardBehaviorState> expectedBehaviors = {
		ELRGuardBehaviorState::IdlePatrol,
		ELRGuardBehaviorState::Suspicious,
		ELRGuardBehaviorState::Investigate,
		ELRGuardBehaviorState::Chase,
		ELRGuardBehaviorState::Stunned
	};
	const TArray<FName> expectedNames = {
		FName(TEXT("IdlePatrol")), FName(TEXT("Suspicious")), FName(TEXT("Investigate")),
		FName(TEXT("Chase")), FName(TEXT("Stunned"))
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
		TestTrue(TEXT("Behavior state condition type"),
			conditionNode.GetScriptStruct() == FLRGuardStateCondition::StaticStruct());
		if (conditionNode.GetScriptStruct() == FLRGuardStateCondition::StaticStruct())
		{
			TestEqual(TEXT("Behavior state condition enum"),
				conditionNode.Get<const FLRGuardStateCondition>().ExpectedBehavior,
				expectedBehaviors[behaviorIndex]);
		}

		const FConstStructView taskNode = stateTree->GetNode(state.TasksBegin);
		TestTrue(TEXT("Behavior state task type"), taskNode.GetScriptStruct() == FLRGuardBehaviorTask::StaticStruct());
		if (taskNode.GetScriptStruct() == FLRGuardBehaviorTask::StaticStruct())
		{
			TestEqual(TEXT("Behavior state task enum"), taskNode.Get<const FLRGuardBehaviorTask>().Behavior,
				expectedBehaviors[behaviorIndex]);
		}

		const FCompactStateTransition* transition = stateTree->GetTransitionFromIndex(FStateTreeIndex16(state.TransitionsBegin));
		if (!TestNotNull(TEXT("BehaviorChanged transition exists"), transition))
		{
			continue;
		}
		TestEqual(TEXT("Transition trigger is OnEvent"), transition->Trigger, EStateTreeTransitionTrigger::OnEvent);
		TestTrue(TEXT("Transition event is BehaviorChanged"), transition->RequiredEvent.Tag == LRGameplayTags::AIEventBehaviorChanged);
		TestEqual(TEXT("Transition targets Root"), transition->State, FStateTreeStateHandle::Root);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRGuardStateTreePersistentRunningTest, "LostRunic.AI.GuardStateTreePersistentRunning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLRGuardStateTreePersistentRunningTest::RunTest(const FString& parameters)
{
	(void)parameters;
	UStateTree* stateTree = LoadObject<UStateTree>(nullptr, TEXT("/Game/LostRunic/Blueprints/Guard/ST_Guard.ST_Guard"));
	if (!TestNotNull(TEXT("Guard StateTree asset loads"), stateTree) || !TestNotNull(TEXT("Engine exists"), GEngine))
	{
		return false;
	}

	const FName worldName = MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass(), TEXT("GuardStateTreeRunningWorld"));
	UWorld* world = UWorld::CreateWorld(EWorldType::Game, false, worldName, GetTransientPackage());
	if (!TestNotNull(TEXT("StateTree test world creates"), world))
	{
		return false;
	}
	FWorldContext& worldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	worldContext.SetCurrentWorld(world);
	FActorSpawnParameters spawnParameters;
	spawnParameters.ObjectFlags = RF_Transient;
	ALRGuardCharacter* guard = world->SpawnActor<ALRGuardCharacter>(spawnParameters);
	ALRGuardAIController* controller = world->SpawnActor<ALRGuardAIController>(spawnParameters);
	UStateTreeAIComponent* stateTreeAI = controller ? controller->FindComponentByClass<UStateTreeAIComponent>() : nullptr;
	bool bPassed = TestNotNull(TEXT("Guard spawns"), guard)
		&& TestNotNull(TEXT("Controller spawns"), controller)
		&& TestNotNull(TEXT("StateTree component exists"), stateTreeAI);
	if (bPassed)
	{
		controller->Possess(guard);
		stateTreeAI->SetStartLogicAutomatically(false);
		stateTreeAI->SetStateTree(stateTree);
		stateTreeAI->StartLogic();
		bPassed &= TestEqual(TEXT("StateTree starts Running"), stateTreeAI->GetStateTreeRunStatus(), EStateTreeRunStatus::Running);

		for (int32 updateIndex = 0; updateIndex < 3; ++updateIndex)
		{
			world->Tick(LEVELTICK_All, 0.016f);
			bPassed &= TestEqual(TEXT("StateTree remains Running"), stateTreeAI->GetStateTreeRunStatus(), EStateTreeRunStatus::Running);
		}

		FLRGuardNoiseStimulus stimulus;
		stimulus.Source = guard;
		stimulus.Location = FVector(100.0f, 0.0f, 0.0f);
		stimulus.Reason = LRGameplayTags::NoiseFootstepRunIndoor;
		stimulus.PropagationMode = ELRGuardNoisePropagationMode::CurrentRoom;
		stimulus.SourcePace = ELRMovementPace::Run;
		stimulus.bHasSourcePace = true;
		stimulus.TimeSeconds = world->GetTimeSeconds();
		controller->ReceiveNoiseStimulus(stimulus);
		bPassed &= TestEqual(TEXT("Alert five resolves Suspicious"), controller->GetResolvedBehavior(),
			ELRGuardBehaviorState::Suspicious);
		stateTreeAI->SendStateTreeEvent(LRGameplayTags::AIEventBehaviorChanged, FConstStructView(), FName());
		stateTreeAI->TickComponent(0.016f, LEVELTICK_All, nullptr);

		world->Tick(LEVELTICK_All, 0.31f);
		stimulus.Location = FVector(180.0f, 0.0f, 0.0f);
		stimulus.TimeSeconds = world->GetTimeSeconds();
		controller->ReceiveNoiseStimulus(stimulus);
		bPassed &= TestEqual(TEXT("Alert six resolves Investigate"), controller->GetResolvedBehavior(),
			ELRGuardBehaviorState::Investigate);
		bPassed &= TestNotEqual(TEXT("No runtime path returns deprecated Search"),
			static_cast<uint8>(controller->GetResolvedBehavior()),
			static_cast<uint8>(ELRGuardBehaviorState::Search_DEPRECATED));
	}

	if (controller)
	{
		controller->UnPossess();
	}
	world->EndPlay(EEndPlayReason::Quit);
	GEngine->DestroyWorldContext(world);
	world->DestroyWorld(false);
	return bPassed;
}

#endif
