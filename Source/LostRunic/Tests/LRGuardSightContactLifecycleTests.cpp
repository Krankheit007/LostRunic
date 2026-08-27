/**
 * @file LRGuardSightContactLifecycleTests.cpp
 * @brief Regression coverage for raw UE Sight contact versus project Visibility gates.
 */
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AI/LRGuardAIController.h"
#include "AI/LRGuardCharacter.h"
#include "AI/LRGuardKnowledgeComponent.h"
#include "AI/LRGuardPerceptionRules.h"
#include "Data/LRGuardTuning.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Framework/LRCharacter.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "TimerManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRGuardSightContactLifecycleRuntimeTest,
	"LostRunic.AI.GuardSightContactLifecycleRuntime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRGuardSightContactLifecycleRuntimeTest::RunTest(const FString& parameters)
{
	if (!TestNotNull(TEXT("Engine exists"), GEngine))
	{
		return false;
	}
	const FName worldName = MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass(),
		TEXT("GuardSightContactLifecycleWorld"));
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
		UAISenseConfig_Sight* sight = NewObject<UAISenseConfig_Sight>(controller);
		sight->SightRadius = 500.0f;
		sight->PeripheralVisionAngleDegrees = 22.5f;
		controller->AIPerception->ConfigureSense(*sight);
		controller->Possess(guard);
		ULRGuardKnowledgeComponent* knowledge = guard->GetKnowledgeComponent();
		const FLRGuardTuningSettings& tuning = controller->Tuning;
		guard->SetActorLocation(FVector::ZeroVector);
		guard->SetActorRotation(FRotator::ZeroRotator);
		player->SetActorLocation(FVector(450.0f, 0.0f, 0.0f));
		const FLRGuardVisibilityResult activeSample = LRGuardPerceptionRules::EvaluateVisibility(
			450.0f, 1.0f, sight->SightRadius, sight->PeripheralVisionAngleDegrees,
			true, true, true, 1.0f, 1.0f, 1.0f, 1.0f, tuning);
		knowledge->SetVisualCandidate(player);
		knowledge->ApplyVisibilitySample(activeSample, 0.5f, tuning);
		controller->PerceivedSightContact = player;
		controller->LastDetectionSampleTime = 0.0;
		controller->StartDetectionSampling();

		player->SetActorLocation(FVector(550.0f, 0.0f, 0.0f));
		controller->HandleDetectionSample();
		const FLRGuardKnowledgeSnapshot hiddenSnapshot = knowledge->GetSnapshot();
		bPassed &= TestEqual(TEXT("Custom inactive visibility keeps the visual candidate"),
			hiddenSnapshot.VisualCandidate.Get(), static_cast<AActor*>(player));
		bPassed &= TestEqual(TEXT("Custom inactive visibility keeps the raw perception contact"),
			controller->PerceivedSightContact.Get(), static_cast<AActor*>(player));
		bPassed &= TestTrue(TEXT("Custom inactive visibility decays exposure"),
			hiddenSnapshot.EffectiveExposureSeconds < 0.5f);

		player->SetActorLocation(FVector(450.0f, 0.0f, 0.0f));
		controller->HandleDetectionSample();
		const FLRGuardKnowledgeSnapshot reacquiredSnapshot = knowledge->GetSnapshot();
		bPassed &= TestTrue(TEXT("The same raw contact reacquires after custom visibility recovers"),
			reacquiredSnapshot.CurrentVisibility.IsActive());
		bPassed &= TestTrue(TEXT("Reacquisition integrates exposure again"),
			reacquiredSnapshot.EffectiveExposureSeconds > hiddenSnapshot.EffectiveExposureSeconds);

		controller->HandleSightLost(player, player->GetActorLocation());
		bPassed &= TestFalse(TEXT("True UE Sight Lost clears the visual candidate"), knowledge->HasVisualCandidate());
		bPassed &= TestTrue(TEXT("True UE Sight Lost keeps residual exposure for decay"),
			knowledge->GetEffectiveExposureSeconds() > 0.0f);
		for (int32 sampleIndex = 0; sampleIndex < 16; ++sampleIndex)
		{
			controller->HandleDetectionSample();
		}
		bPassed &= TestEqual(TEXT("Residual exposure decays to zero after true Sight Lost"),
			knowledge->GetEffectiveExposureSeconds(), 0.0f, 0.001f);
		bPassed &= TestFalse(TEXT("Detection timer stops after contact and exposure end"),
			world->GetTimerManager().IsTimerActive(controller->DetectionSampleTimer));
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
