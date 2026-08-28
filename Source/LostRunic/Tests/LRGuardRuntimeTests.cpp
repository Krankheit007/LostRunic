/**
 * @file LRGuardRuntimeTests.cpp
 * @brief Controller 入口的 Guard 警戒序列回归测试。
 */
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AI/LRAlertComponent.h"
#include "AI/LRGuardAIController.h"
#include "AI/LRGuardCharacter.h"
#include "AI/LRGuardKnowledgeComponent.h"
#include "Core/LRGameplayTags.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRGuardAwarenessRuntimeContractTest,
	"LostRunic.AI.GuardAwarenessRuntimeContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRGuardAwarenessRuntimeContractTest::RunTest(const FString& parameters)
{
	(void)parameters;
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
	bool bPassed = TestNotNull(TEXT("Guard spawns"), guard)
		&& TestNotNull(TEXT("Controller spawns"), controller);
	if (bPassed)
	{
		controller->Possess(guard);
		const FVector location(100.0f, 0.0f, 0.0f);
		auto SendRun = [controller, world, location]()
		{
			FLRGuardNoiseStimulus stimulus;
			stimulus.Source = controller->GetPawn();
			stimulus.Location = location;
			stimulus.Reason = LRGameplayTags::NoiseFootstepRunIndoor;
			stimulus.PropagationMode = ELRGuardNoisePropagationMode::CurrentRoom;
			stimulus.SourcePace = ELRMovementPace::Run;
			stimulus.bHasSourcePace = true;
			stimulus.TimeSeconds = world->GetTimeSeconds();
			controller->ReceiveNoiseStimulus(stimulus);
		};
		auto Advance = [world](const float seconds)
		{
			world->Tick(LEVELTICK_All, seconds);
		};

		SendRun();
		bPassed &= TestEqual(TEXT("First current-room run jumps to Floor five"),
			controller->GetAlertComponent()->GetAlertLevel(), 5);
		bPassed &= TestEqual(TEXT("Floor five resolves Suspicious"),
			controller->GetResolvedBehavior(), ELRGuardBehaviorState::Suspicious);

		SendRun();
		bPassed &= TestEqual(TEXT("White first cooldown freezes alert at five"),
			controller->GetAlertComponent()->GetAlertLevel(), 5);
		Advance(0.31f);
		SendRun();
		bPassed &= TestEqual(TEXT("After white cooldown run crosses five to six"),
			controller->GetAlertComponent()->GetAlertLevel(), 6);
		bPassed &= TestEqual(TEXT("Six resolves Investigate"), controller->GetResolvedBehavior(),
			ELRGuardBehaviorState::Investigate);
		Advance(0.13f);
		SendRun();
		bPassed &= TestEqual(TEXT("After red first cooldown run reaches seven"),
			controller->GetAlertComponent()->GetAlertLevel(), 7);

		for (int32 expectedLevel = 8; expectedLevel <= 10; ++expectedLevel)
		{
			Advance(0.21f);
			SendRun();
			bPassed &= TestEqual(TEXT("Repeated run raises red alert by one"),
				controller->GetAlertComponent()->GetAlertLevel(), expectedLevel);
		}
		Advance(0.21f);
		SendRun();
		bPassed &= TestEqual(TEXT("Noise cannot enter Chase level eleven"),
			controller->GetAlertComponent()->GetAlertLevel(), 10);
		bPassed &= TestNotEqual(TEXT("Noise-only alert never resolves deprecated Search"),
			static_cast<uint8>(controller->GetResolvedBehavior()),
			static_cast<uint8>(ELRGuardBehaviorState::Search_DEPRECATED));

		ULRGuardKnowledgeComponent* knowledge = guard->GetKnowledgeComponent();
		ULRAlertComponent* alert = guard->GetAlertComponent();
		const FVector disturbance = knowledge->GetLastDisturbanceLocation();
		const int32 alertBeforeUnPossess = alert->GetAlertLevel();
		controller->UnPossess();
		bPassed &= TestEqual(TEXT("UnPossess preserves authoritative alert value"),
			alert->GetAlertLevel(), alertBeforeUnPossess);
		bPassed &= TestEqual(TEXT("UnPossess preserves disturbance memory"),
			knowledge->GetLastDisturbanceLocation(), disturbance);
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
