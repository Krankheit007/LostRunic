/**
 * @file LRGuardSightContactLifecycleTests.cpp
 * @brief Raw UE Sight Contact、Hard Hidden、Grace 和确认记忆的回归测试。
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
#include "Framework/LRCharacter.h"
#include "Stealth/LRHideComponent.h"
#include "Stealth/LRHidePoint.h"
#include "TimerManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRGuardSightContactLifecycleRuntimeTest,
	"LostRunic.AI.GuardSightContactLifecycleRuntime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRGuardSightContactLifecycleRuntimeTest::RunTest(const FString& parameters)
{
	(void)parameters;
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
	ALRHidePoint* hidePoint = world->SpawnActor<ALRHidePoint>(spawnParameters);
	bool bPassed = TestNotNull(TEXT("Guard spawns"), guard)
		&& TestNotNull(TEXT("Controller spawns"), controller)
		&& TestNotNull(TEXT("Player spawns"), player)
		&& TestNotNull(TEXT("Hide point spawns"), hidePoint);
	if (bPassed)
	{
		guard->SetActorLocation(FVector::ZeroVector);
		player->SetActorLocation(FVector(450.0f, 0.0f, 0.0f));
		hidePoint->SetActorLocation(player->GetActorLocation());
		controller->Possess(guard);

		ULRGuardKnowledgeComponent* knowledge = guard->GetKnowledgeComponent();
		ULRAlertComponent* alert = guard->GetAlertComponent();
		controller->RawSightContact = player;
		controller->bHasRawSightContact = true;
		knowledge->SetVisualCandidate(player);
		controller->StartSightTracking();
		controller->HandleSightAcquiredOrTracked(player);

		bPassed &= TestEqual(TEXT("Low-alert Sight enters red level six"), alert->GetAlertLevel(), 6);
		bPassed &= TestTrue(TEXT("Initial Sight starts Grace"), controller->IsSightToChaseGraceActive());
		bPassed &= TestTrue(TEXT("Raw contact starts Sight tracking timer"),
			world->GetTimerManager().IsTimerActive(controller->SightTrackingTimer));

		FLRGuardNoiseStimulus noise;
		noise.Source = player;
		noise.Location = FVector(350.0f, 100.0f, 0.0f);
		noise.Reason = LRGameplayTags::NoiseInteraction;
		noise.TimeSeconds = world->GetTimeSeconds();
		controller->ReceiveNoiseStimulus(noise);
		FLRGuardKnowledgeSnapshot duringGrace = knowledge->GetSnapshot();
		bPassed &= TestEqual(TEXT("Noise during Sight Grace cannot raise Alert"), alert->GetAlertLevel(), 6);
		bPassed &= TestTrue(TEXT("Noise during Sight Grace is remembered"),
			duringGrace.LastDisturbanceLocation.Equals(noise.Location));
		bPassed &= TestTrue(TEXT("Sight target keeps investigation priority during Grace"),
			duringGrace.LatestInvestigationLocation.Equals(player->GetActorLocation()));
		bPassed &= TestFalse(TEXT("Grace does not start RedObserve"), alert->IsObserving());

		controller->MarkInvestigationReached();
		bPassed &= TestFalse(TEXT("Arrival during Grace does not start RedObserve"), alert->IsObserving());
		controller->HandleSightGraceExpired();
		FLRGuardKnowledgeSnapshot confirmed = knowledge->GetSnapshot();
		bPassed &= TestEqual(TEXT("Grace expiry while visible confirms eleven"), alert->GetAlertLevel(), 11);
		bPassed &= TestTrue(TEXT("Grace expiry records ConfirmedThreat"), confirmed.bHasConfirmedThreat);
		bPassed &= TestFalse(TEXT("Grace is no longer active after confirmation"),
			controller->IsSightToChaseGraceActive());

		ULRHideComponent* hide = player->GetHideComponent();
		if (hide)
		{
			hide->BeginPlay();
			bPassed &= TestTrue(TEXT("Player can enter Hard Hidden for the lifecycle test"),
				hide->EnterHidePoint(hidePoint));
			controller->HandleSightTracking();
			const FLRGuardKnowledgeSnapshot hidden = knowledge->GetSnapshot();
			bPassed &= TestTrue(TEXT("Hard Hidden keeps raw UE Sight contact"), controller->HasRawSightContact());
			bPassed &= TestTrue(TEXT("Hard Hidden keeps Sight tracking timer active"),
			world->GetTimerManager().IsTimerActive(controller->SightTrackingTimer));
			bPassed &= TestFalse(TEXT("Hard Hidden clears effective visibility"), hidden.bCurrentlyVisible);
			bPassed &= TestEqual(TEXT("Hard Hidden lowers confirmed alert to ten"), alert->GetAlertLevel(), 10);
			bPassed &= TestTrue(TEXT("Hard Hidden keeps confirmed threat memory"), hidden.bHasConfirmedThreat);

			hide->ExitHidePoint();
			controller->HandleSightTracking();
			bPassed &= TestEqual(TEXT("Leaving Hard Hidden reacquires confirmed sight immediately"),
				alert->GetAlertLevel(), 11);
		}

		controller->HandleSightLost(player, player->GetActorLocation());
		const FLRGuardKnowledgeSnapshot lost = knowledge->GetSnapshot();
		bPassed &= TestFalse(TEXT("Raw UE Sight Lost clears raw contact"), controller->HasRawSightContact());
		bPassed &= TestFalse(TEXT("Raw UE Sight Lost stops tracking timer"),
			world->GetTimerManager().IsTimerActive(controller->SightTrackingTimer));
		bPassed &= TestFalse(TEXT("Raw UE Sight Lost clears current visibility"), lost.bCurrentlyVisible);
		bPassed &= TestTrue(TEXT("Raw UE Sight Lost clears visual candidate"), !lost.bHasVisualCandidate);
	bPassed &= TestTrue(TEXT("Raw UE Sight Lost retains confirmed threat memory"), lost.bHasConfirmedThreat);
		bPassed &= TestEqual(TEXT("Raw UE Sight Lost returns to investigate level ten"), alert->GetAlertLevel(), 10);
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
