/**
 * @file LRGuardRuntimeTests.cpp
 * @brief Focused controller-boundary regression tests for Guard Awareness.
 */
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AI/LRGuardAIController.h"
#include "AI/LRAlertComponent.h"
#include "AI/LRGuardKnowledgeComponent.h"
#include "AI/LRGuardCharacter.h"
#include "Core/LRGameplayTags.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Framework/LRCharacter.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRGuardAwarenessRuntimeContractTest,
	"LostRunic.AI.GuardAwarenessRuntimeContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRGuardAwarenessRuntimeContractTest::RunTest(const FString& parameters)
{
	if (!TestNotNull(TEXT("Engine exists"), GEngine))
	{
		return false;
	}
	const FName worldName = MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass(),
		TEXT("GuardAwarenessRuntimeWorld"));
	UWorld* world = UWorld::CreateWorld(EWorldType::Game, false, worldName, GetTransientPackage());
	if (!TestNotNull(TEXT("Test world creates"), world))
	{
		return false;
	}
	FWorldContext& worldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	worldContext.SetCurrentWorld(world);
	FActorSpawnParameters spawnParameters;
	spawnParameters.ObjectFlags = RF_Transient;
	ALRGuardCharacter* guard = world->SpawnActor<ALRGuardCharacter>(spawnParameters);
	ALRGuardAIController* controller = world->SpawnActor<ALRGuardAIController>(spawnParameters);
	ALRCharacter* player = world->SpawnActor<ALRCharacter>(spawnParameters);
	bool bPassed = TestNotNull(TEXT("Guard spawns"), guard)
		&& TestNotNull(TEXT("Controller spawns"), controller)
		&& TestNotNull(TEXT("Player spawns"), player);
	if (bPassed)
	{
		controller->Possess(guard);
		bPassed &= TestTrue(TEXT("Spawned player passes IsValid"), IsValid(player));
		bPassed &= TestTrue(TEXT("Player class reflection registers perception target"),
			player->GetClass()->ImplementsInterface(ULRGuardPerceptionTarget::StaticClass()));
		bPassed &= TestTrue(TEXT("Player native perception target implementation returns true"),
			CastChecked<ILRGuardPerceptionTarget>(player)->IsRelevantGuardSightTarget_Implementation());
		bPassed &= TestTrue(TEXT("Player is a relevant sight target"),
			controller->IsRelevantSightTarget(player));
		bPassed &= TestFalse(TEXT("Guard is not a relevant player sight target"),
			controller->IsRelevantSightTarget(guard));

		auto SendNoise = [controller, player, world](const FVector& location,
			const FGameplayTag reason, const ELRGuardNoisePropagationMode mode)
		{
			FLRGuardNoiseStimulus stimulus;
			stimulus.Source = player;
			stimulus.Location = location;
			stimulus.Reason = reason;
			stimulus.PropagationMode = mode;
			stimulus.TimeSeconds = world->GetTimeSeconds();
			controller->ReceiveNoiseStimulus(stimulus);
		};

		const FVector firstLocation(100.0f, 0.0f, 0.0f);
		SendNoise(firstLocation, LRGameplayTags::NoiseFootstepRunIndoor,
			ELRGuardNoisePropagationMode::CurrentRoom);
		const FLRGuardAwarenessSnapshot beforeRejected = controller->GetAwarenessSnapshot();
		SendNoise(FVector(999.0f), LRGameplayTags::NoiseFootstepWalkFaint,
			ELRGuardNoisePropagationMode::Hearing);
		const FLRGuardAwarenessSnapshot afterRejected = controller->GetAwarenessSnapshot();
		bPassed &= TestEqual(TEXT("Rejected faint walk does not change Alert"),
			afterRejected.Alert.Level, beforeRejected.Alert.Level);
		bPassed &= TestEqual(TEXT("Rejected faint walk does not commit evidence revision"),
			afterRejected.Knowledge.InvestigationContextRevision,
			beforeRejected.Knowledge.InvestigationContextRevision);

		const FVector secondLocation(250.0f, 0.0f, 0.0f);
		SendNoise(secondLocation, LRGameplayTags::NoiseFootstepRunIndoor,
			ELRGuardNoisePropagationMode::AdjacentRoom);
		bPassed &= TestEqual(TEXT("Immediate navigation failure resolves Search"),
			controller->GetResolvedBehavior(), ELRGuardBehaviorState::Search);
		bPassed &= TestEqual(TEXT("Accepted evidence owns investigation location"),
			controller->GetAwarenessSnapshot().InvestigationLocation, secondLocation);

		for (int32 step = 0; step < 5; ++step)
		{
			SendNoise(secondLocation, LRGameplayTags::NoiseFootstepRunIndoor,
				ELRGuardNoisePropagationMode::AdjacentRoom);
		}
		bPassed &= TestEqual(TEXT("Alert reaches maximum"),
			controller->GetAwarenessSnapshot().Alert.Level, 11);
		bPassed &= TestNotEqual(TEXT("Alert 11 without confirmed threat never chases"),
			controller->GetResolvedBehavior(), ELRGuardBehaviorState::Chase);
		controller->MarkInvestigationReached();
		bPassed &= TestEqual(TEXT("Reached evidence at Alert 11 resolves Search"),
			controller->GetResolvedBehavior(), ELRGuardBehaviorState::Search);
		ULRGuardKnowledgeComponent* pawnKnowledge = guard->GetKnowledgeComponent();
		ULRAlertComponent* pawnAlert = guard->GetAlertComponent();
		const FLRGuardKnowledgeSnapshot knowledgeBeforeUnPossess = pawnKnowledge->GetSnapshot();
		const FLRAlertSnapshot alertBeforeUnPossess = pawnAlert->GetAlertSnapshot();
		controller->UnPossess();
		const FLRGuardKnowledgeSnapshot knowledgeAfterUnPossess = pawnKnowledge->GetSnapshot();
		bPassed &= TestEqual(TEXT("UnPossess clears transient exposure"), knowledgeAfterUnPossess.EffectiveExposureSeconds, 0.0f, 0.001f);
		bPassed &= TestEqual(TEXT("UnPossess clears transient detection stage"), knowledgeAfterUnPossess.Stage, ELRGuardDetectionStage::None);
		bPassed &= TestEqual(TEXT("UnPossess preserves disturbance memory"), knowledgeAfterUnPossess.LastDisturbanceLocation, knowledgeBeforeUnPossess.LastDisturbanceLocation);
		bPassed &= TestEqual(TEXT("UnPossess preserves pending memory"), knowledgeAfterUnPossess.bPendingThreatInvestigation, knowledgeBeforeUnPossess.bPendingThreatInvestigation);
		bPassed &= TestEqual(TEXT("UnPossess preserves investigation revision"), knowledgeAfterUnPossess.InvestigationContextRevision, knowledgeBeforeUnPossess.InvestigationContextRevision);
		bPassed &= TestEqual(TEXT("UnPossess preserves Alert"), pawnAlert->GetAlertLevel(), alertBeforeUnPossess.Level);
		bPassed &= TestNull(TEXT("UnPossess clears controller Knowledge reference"),
			controller->GetKnowledgeComponent());
	}
	world->EndPlay(EEndPlayReason::Quit);
	GEngine->DestroyWorldContext(world);
	world->DestroyWorld(false);
	return bPassed;
}

#endif
