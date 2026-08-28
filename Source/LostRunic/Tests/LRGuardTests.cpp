/**
 * @file LRGuardTests.cpp
 * @brief Guard 0-11 警戒、噪声 Floor/CD、行为档位和组件 Tick 契约测试。
 */
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AI/LRAlertComponent.h"
#include "AI/LRAlertRules.h"
#include "AI/LRGuardAIController.h"
#include "AI/LRGuardCharacter.h"
#include "AI/LRGuardPerceptionRules.h"
#include "Core/LRGameplayTags.h"
#include "Data/LRGuardTuning.h"
#include "Stealth/LRHideComponent.h"
#include "Stealth/LRNoiseEmitterComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRAlertRulesTest, "LostRunic.AI.AlertLevelsAndBehaviorBands",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRAlertRulesTest::RunTest(const FString& parameters)
{
	(void)parameters;
	TestEqual(TEXT("Alert clamps below zero"), LRAlertRules::ApplyDelta(2, -8), 0);
	TestEqual(TEXT("Alert clamps at eleven"), LRAlertRules::ApplyDelta(9, 8), 11);
	TestEqual(TEXT("Zero resolves IdlePatrol"), LRAlertRules::ResolveState(0), ELRGuardBehaviorState::IdlePatrol);
	TestEqual(TEXT("One to five resolves Suspicious"), LRAlertRules::ResolveState(5), ELRGuardBehaviorState::Suspicious);
	TestEqual(TEXT("Six to ten resolves Investigate"), LRAlertRules::ResolveState(6), ELRGuardBehaviorState::Investigate);
	TestEqual(TEXT("Ten remains Investigate"), LRAlertRules::ResolveState(10), ELRGuardBehaviorState::Investigate);
	TestEqual(TEXT("Alert eleven alone never authorizes Chase"),
		LRAlertRules::ResolveState(11), ELRGuardBehaviorState::Investigate);
	FLRAlertSnapshot emptyAlert;
	FLRGuardKnowledgeSnapshot emptyKnowledge;
	TestEqual(TEXT("Stun overrides every alert band"),
		LRAlertRules::ResolveTargetBehavior(true, emptyAlert, emptyKnowledge),
		ELRGuardBehaviorState::Stunned);
	TestNotEqual(TEXT("Runtime resolver never returns deprecated Search slot"),
		static_cast<uint8>(LRAlertRules::ResolveState(6)),
		static_cast<uint8>(ELRGuardBehaviorState::Search_DEPRECATED));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRNoiseAlertDeltaTest, "LostRunic.AI.NoiseAlertDelta",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRNoiseAlertDeltaTest::RunTest(const FString& parameters)
{
	(void)parameters;
	const FLRGuardTuningSettings tuning;

	const FLRNoiseResponse indoorRun = LRGuardPerceptionRules::ResolveNoiseAlertDelta(
		LRGameplayTags::NoiseFootstepRunIndoor, ELRGuardNoisePropagationMode::CurrentRoom, 0, tuning);
	TestTrue(TEXT("Current-room run responds"), indoorRun.bRespond);
	TestTrue(TEXT("Current-room run uses a Floor"), indoorRun.bUseCurrentRoomRunFloor);
	TestEqual(TEXT("Current-room run uses normal increase after Floor"), indoorRun.Delta,
		tuning.AttractAlertAmount);

	const FLRNoiseResponse adjacentRun = LRGuardPerceptionRules::ResolveNoiseAlertDelta(
		LRGameplayTags::NoiseFootstepRunIndoor, ELRGuardNoisePropagationMode::AdjacentRoom, 0, tuning);
	TestTrue(TEXT("Adjacent-room run responds"), adjacentRun.bRespond);
	TestEqual(TEXT("Adjacent-room run uses configured increase"), adjacentRun.Delta,
		tuning.AdjacentRoomRunAlertAmount);

	const FLRNoiseResponse faintLow = LRGuardPerceptionRules::ResolveNoiseAlertDelta(
		LRGameplayTags::NoiseFootstepWalkFaint, ELRGuardNoisePropagationMode::Hearing, 5, tuning);
	TestFalse(TEXT("Faint walk is ignored below investigate band"), faintLow.bRespond);
	const FLRNoiseResponse faintHigh = LRGuardPerceptionRules::ResolveNoiseAlertDelta(
		LRGameplayTags::NoiseFootstepWalkFaint, ELRGuardNoisePropagationMode::Hearing, 6, tuning);
	TestTrue(TEXT("Faint walk responds in investigate band"), faintHigh.bRespond);

	const FLRNoiseResponse ordinary = LRGuardPerceptionRules::ResolveNoiseAlertDelta(
		LRGameplayTags::NoiseInteraction, ELRGuardNoisePropagationMode::Hearing, 4, tuning);
	TestTrue(TEXT("Ordinary noise responds"), ordinary.bRespond);
	TestEqual(TEXT("Ordinary noise uses attract amount"), ordinary.Delta, tuning.AttractAlertAmount);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRRoomRunAlertSequenceTest, "LostRunic.AI.RoomRunAlertFloorSequence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRRoomRunAlertSequenceTest::RunTest(const FString& parameters)
{
	(void)parameters;
	const FLRGuardTuningSettings tuning;
	int32 alert = 0;
	const FLRNoiseResponse run = LRGuardPerceptionRules::ResolveNoiseAlertDelta(
		LRGameplayTags::NoiseFootstepRunIndoor, ELRGuardNoisePropagationMode::CurrentRoom, alert, tuning);

	alert = LRGuardPerceptionRules::ResolveNoiseResultLevel(alert, run, tuning);
	TestEqual(TEXT("Run starts at RoomRunAlertLevel"), alert, 5);
	alert = LRGuardPerceptionRules::ResolveNoiseResultLevel(alert, run, tuning);
	TestEqual(TEXT("Run at Floor becomes six"), alert, 6);
	alert = LRGuardPerceptionRules::ResolveNoiseResultLevel(alert, run, tuning);
	TestEqual(TEXT("Run continues to seven"), alert, 7);
	alert = LRGuardPerceptionRules::ResolveNoiseResultLevel(9, run, tuning);
	TestEqual(TEXT("Run reaches red cap ten"), alert, 10);
	alert = LRGuardPerceptionRules::ResolveNoiseResultLevel(10, run, tuning);
	TestEqual(TEXT("Noise cannot enter eleven"), alert, 10);

	const FLRNoiseResponse ordinary = LRGuardPerceptionRules::ResolveNoiseAlertDelta(
		LRGameplayTags::NoiseInteraction, ELRGuardNoisePropagationMode::Hearing, 0, tuning);
	TestEqual(TEXT("Ordinary noise starts at one"),
		LRGuardPerceptionRules::ResolveNoiseResultLevel(0, ordinary, tuning), 1);
	TestEqual(TEXT("Ordinary noise crosses five to six"),
		LRGuardPerceptionRules::ResolveNoiseResultLevel(5, ordinary, tuning), 6);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRAlertIncreaseCooldownTest, "LostRunic.AI.AlertIncreaseCooldown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRAlertIncreaseCooldownTest::RunTest(const FString& parameters)
{
	(void)parameters;
	const FLRGuardTuningSettings tuning;
	TestEqual(TEXT("First white run uses pace multiplier"),
		LRAlertRules::ResolveAttractCooldown(5, true, ELRMovementPace::Run, true, tuning), 0.3f, 0.001f);
	TestEqual(TEXT("First red run uses red base and pace multiplier"),
		LRAlertRules::ResolveAttractCooldown(6, true, ELRMovementPace::Run, true, tuning), 0.12f, 0.001f);
	TestEqual(TEXT("First white sneak uses configured multiplier"),
		LRAlertRules::ResolveAttractCooldown(1, true, ELRMovementPace::Sneak, true, tuning), 0.8f, 0.001f);
	TestEqual(TEXT("Later red noise uses fixed red cooldown"),
		LRAlertRules::ResolveAttractCooldown(7, false, ELRMovementPace::Run, true, tuning), 0.2f, 0.001f);
	TestEqual(TEXT("Non-footstep noise uses neutral first cooldown"),
		LRAlertRules::ResolveAttractCooldown(6, true, ELRMovementPace::Run, false, tuning), 0.2f, 0.001f);
	TestTrue(TEXT("Cooldown boundary allows increase"), LRAlertRules::IsIncreaseAllowed(10.0, 9.5, 0.5f));
	TestFalse(TEXT("Active cooldown rejects increase"), LRAlertRules::IsIncreaseAllowed(10.0, 9.6, 0.5f));
	TestTrue(TEXT("Zero cooldown always allows increase"), LRAlertRules::IsIncreaseAllowed(10.0, 0.0, 0.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRAlertTierTest, "LostRunic.AI.AlertTierMapping",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRAlertTierTest::RunTest(const FString& parameters)
{
	(void)parameters;
	TestEqual(TEXT("Zero alert is hidden"), LRAlertRules::ResolveAlertTier(0), ELRGuardAlertTier::Hidden);
	TestEqual(TEXT("One is white"), LRAlertRules::ResolveAlertTier(1), ELRGuardAlertTier::White);
	TestEqual(TEXT("Five is white boundary"), LRAlertRules::ResolveAlertTier(5), ELRGuardAlertTier::White);
	TestEqual(TEXT("Six is red"), LRAlertRules::ResolveAlertTier(6), ELRGuardAlertTier::Red);
	TestEqual(TEXT("Ten is red boundary"), LRAlertRules::ResolveAlertTier(10), ELRGuardAlertTier::Red);
	TestEqual(TEXT("Eleven is full red"), LRAlertRules::ResolveAlertTier(11), ELRGuardAlertTier::Full);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRAlertComponentSnapshotTest, "LostRunic.AI.AlertComponentReadOnlySnapshot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRAlertComponentSnapshotTest::RunTest(const FString& parameters)
{
	(void)parameters;
	const ULRAlertComponent* alert = NewObject<ULRAlertComponent>(GetTransientPackage());
	if (!TestNotNull(TEXT("Alert component created"), alert))
	{
		return false;
	}
	const FLRAlertSnapshot snapshot = alert->GetAlertSnapshot();
	TestEqual(TEXT("New alert starts at zero"), snapshot.Level, 0);
	TestEqual(TEXT("New alert tier is hidden"), snapshot.Tier, ELRGuardAlertTier::Hidden);
	TestFalse(TEXT("New alert is not full"), snapshot.bFullAlert);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRAlertSnapshotPresentationTest, "LostRunic.AI.AlertSnapshotPresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRAlertSnapshotPresentationTest::RunTest(const FString& parameters)
{
	(void)parameters;
	ULRAlertComponent* alert = NewObject<ULRAlertComponent>(GetTransientPackage());
	if (!TestNotNull(TEXT("Alert component created for presentation snapshot"), alert))
	{
		return false;
	}

	const int32 levels[] = { 0, 1, 5, 6, 10, 11 };
	const float expectedFractions[] = { 0.0f, 0.2f, 1.0f, 0.2f, 1.0f, 1.0f };
	for (int32 index = 0; index < UE_ARRAY_COUNT(levels); ++index)
	{
		alert->AlertLevel = levels[index];
		const FLRAlertSnapshot snapshot = alert->GetAlertSnapshot();
		TestEqual(*FString::Printf(TEXT("Alert %d snapshot fraction"), levels[index]),
			snapshot.Fraction, expectedFractions[index], 0.001f);
	}
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRGuardCompositionTest, "LostRunic.AI.GuardCompositionTickPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRGuardCompositionTest::RunTest(const FString& parameters)
{
	(void)parameters;
	TestFalse(TEXT("Guard actor Tick is disabled"), GetDefault<ALRGuardCharacter>()->PrimaryActorTick.bCanEverTick);
	TestTrue(TEXT("AI controller Tick remains enabled for Focus maintenance"),
		GetDefault<ALRGuardAIController>()->PrimaryActorTick.bCanEverTick);
	TestFalse(TEXT("Alert component Tick is disabled"), GetDefault<ULRAlertComponent>()->PrimaryComponentTick.bCanEverTick);
	TestFalse(TEXT("Hide component Tick is disabled"), GetDefault<ULRHideComponent>()->PrimaryComponentTick.bCanEverTick);
	TestFalse(TEXT("Noise emitter Tick is disabled"), GetDefault<ULRNoiseEmitterComponent>()->PrimaryComponentTick.bCanEverTick);
	return true;
}

#endif
