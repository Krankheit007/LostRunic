/**
 * @file LRGuardAwarenessTests.cpp
 * @brief Guard Awareness/Knowledge 事实快照和行为解析回归测试。
 */
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AI/LRAlertRules.h"
#include "AI/LRGuardKnowledgeComponent.h"
#include "Core/LRGameplayTags.h"
#include "GameFramework/Actor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRGuardAwarenessSnapshotRulesTest, "LostRunic.AI.AwarenessSnapshotRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRGuardAwarenessSnapshotRulesTest::RunTest(const FString& parameters)
{
	(void)parameters;
	TestEqual(TEXT("Stunned overrides zero alert"), LRAlertRules::ResolveTargetBehavior(true, 0),
		ELRGuardBehaviorState::Stunned);
	TestEqual(TEXT("Zero alert is IdlePatrol"), LRAlertRules::ResolveTargetBehavior(false, 0),
		ELRGuardBehaviorState::IdlePatrol);
	TestEqual(TEXT("One to five is Suspicious"), LRAlertRules::ResolveTargetBehavior(false, 1),
		ELRGuardBehaviorState::Suspicious);
	TestEqual(TEXT("Six to ten is Investigate"), LRAlertRules::ResolveTargetBehavior(false, 10),
		ELRGuardBehaviorState::Investigate);
	TestEqual(TEXT("Eleven is Chase"), LRAlertRules::ResolveTargetBehavior(false, 11),
		ELRGuardBehaviorState::Chase);

	FLRGuardKnowledgeSnapshot knowledge;
	TestEqual(TEXT("No latest location resolves to zero"),
		LRAlertRules::ResolveInvestigationLocation(knowledge), FVector::ZeroVector);
	knowledge.bHasLatestInvestigationLocation = true;
	knowledge.LatestInvestigationLocation = FVector(125.0f, 30.0f, 0.0f);
	TestEqual(TEXT("Resolver uses the unique latest investigation location"),
		LRAlertRules::ResolveInvestigationLocation(knowledge), knowledge.LatestInvestigationLocation);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRGuardKnowledgeMemoryTest, "LostRunic.AI.KnowledgeMemoryLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRGuardKnowledgeMemoryTest::RunTest(const FString& parameters)
{
	(void)parameters;
	ULRGuardKnowledgeComponent* knowledge = NewObject<ULRGuardKnowledgeComponent>(GetTransientPackage());
	AActor* player = NewObject<AActor>(GetTransientPackage());
	if (!TestNotNull(TEXT("Knowledge component created"), knowledge)
		|| !TestNotNull(TEXT("Memory actor created"), player))
	{
		return false;
	}

	const FVector sightLocation(100.0f, 25.0f, 0.0f);
	knowledge->RecordVisibleThreat(player, sightLocation);
	FLRGuardKnowledgeSnapshot snapshot = knowledge->GetSnapshot();
	TestTrue(TEXT("Sight records visible candidate"), snapshot.bHasVisualCandidate);
	TestTrue(TEXT("Sight records current visibility"), snapshot.bCurrentlyVisible);
	TestTrue(TEXT("Sight becomes the latest investigation target"), snapshot.LatestInvestigationLocation.Equals(sightLocation));
	knowledge->SetConfirmedThreat(player, sightLocation);
	snapshot = knowledge->GetSnapshot();
	TestTrue(TEXT("Confirmed threat is only set by confirmation"), snapshot.bHasConfirmedThreat);

	FLRGuardNoiseStimulus noise;
	noise.Source = player;
	noise.Location = FVector(250.0f, 40.0f, 0.0f);
	noise.Reason = LRGameplayTags::NoiseInteraction;
	noise.TimeSeconds = 2.0f;
	knowledge->RecordDisturbance(noise, false);
	snapshot = knowledge->GetSnapshot();
	TestTrue(TEXT("Noise history is retained during valid sight"), snapshot.LastDisturbanceLocation.Equals(noise.Location));
	TestTrue(TEXT("Valid sight keeps priority over noise target"), snapshot.LatestInvestigationLocation.Equals(sightLocation));

	knowledge->RecordSightLost(sightLocation);
	snapshot = knowledge->GetSnapshot();
	TestFalse(TEXT("Sight loss clears current visibility"), snapshot.bCurrentlyVisible);
	TestTrue(TEXT("Sight loss returns investigation target to last known sight"),
		snapshot.LatestInvestigationLocation.Equals(sightLocation));
	knowledge->RecordDisturbance(noise, true);
	snapshot = knowledge->GetSnapshot();
	TestTrue(TEXT("Noise retargets after sight is lost"), snapshot.LatestInvestigationLocation.Equals(noise.Location));

	knowledge->ResetAwareness();
	snapshot = knowledge->GetSnapshot();
	TestFalse(TEXT("Alert zero reset clears confirmed threat memory"), snapshot.bHasConfirmedThreat);
	TestFalse(TEXT("Alert zero reset clears investigation location"), snapshot.bHasLatestInvestigationLocation);
	return true;
}

#endif
