/**
 * @file LRTuningTests.cpp
 * @brief Guard 新调优字段和共享调优合法性测试。
 */
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Data/LRGuardTuning.h"
#include "Data/LRInteractionTuning.h"
#include "Data/LRMovementTuning.h"
#include "Data/LRNPCTuning.h"
#include "Data/LRPresentationTuning.h"
#include "Data/LRSaveTuning.h"
#include "Data/LRStateTuning.h"
#include "Data/LRUITuning.h"
#include "Materials/Material.h"
#include "Materials/MaterialParameterCollection.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRTuningDefaultsTest, "LostRunic.Tuning.DefaultsAreValid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRTuningDefaultsTest::RunTest(const FString& parameters)
{
	(void)parameters;
	FString error;
	TestTrue(TEXT("State defaults"), NewObject<ULRStateTuning>()->Validate(error));
	TestTrue(TEXT("Movement defaults"), NewObject<ULRMovementTuning>()->Validate(error));
	TestTrue(TEXT("Interaction defaults"), NewObject<ULRInteractionTuning>()->Validate(error));
	TestTrue(TEXT("Guard defaults"), FLRGuardTuningSettings().Validate(error));

	const FLRGuardTuningSettings guard;
	TestEqual(TEXT("White observation default"), guard.SuspiciousObserveSeconds, 3.0f, 0.001f);
	TestEqual(TEXT("Red observation default"), guard.InvestigateObserveSeconds, 3.0f, 0.001f);
	TestEqual(TEXT("Sight grace default"), guard.SightToChaseGraceSeconds, 0.5f, 0.001f);
	TestEqual(TEXT("White noise cooldown default"), guard.SuspiciousStimulusCooldownSeconds, 0.5f, 0.001f);
	TestEqual(TEXT("Red noise cooldown default"), guard.InvestigateStimulusCooldownSeconds, 0.2f, 0.001f);
	TestEqual(TEXT("Alert decay interval default"), guard.AlertDecayIntervalSeconds, 0.5f, 0.001f);
	TestEqual(TEXT("Room run Floor default"), guard.RoomRunAlertLevel, 5);
	TestEqual(TEXT("Sight tracking interval default"), guard.SightTrackingIntervalSeconds, 0.1f, 0.001f);
	TestEqual(TEXT("First run multiplier default"), guard.FirstAttractRunCooldownMultiplier, 0.6f, 0.001f);
	TestEqual(TEXT("First walk multiplier default"), guard.FirstAttractWalkCooldownMultiplier, 1.0f, 0.001f);
	TestEqual(TEXT("First sneak multiplier default"), guard.FirstAttractSneakCooldownMultiplier, 1.6f, 0.001f);
	TestEqual(TEXT("Investigate retarget distance default"), guard.InvestigateMoveRetargetDistanceCm, 75.0f, 0.001f);
	TestEqual(TEXT("Investigate speed default"), guard.InvestigateSpeed, 170.0f, 0.001f);
	TestEqual(TEXT("Chase speed default"), guard.ChaseSpeed, 300.0f, 0.001f);

	TestTrue(TEXT("Save defaults"), NewObject<ULRSaveTuning>()->Validate(error));
	TestTrue(TEXT("UI defaults"), NewObject<ULRUITuning>()->Validate(error));
	ULRPresentationTuning* presentation = NewObject<ULRPresentationTuning>();
	presentation->PerceptionCompositeMaterial =
		TSoftObjectPtr<UMaterialInterface>(NewObject<UMaterial>(presentation));
	presentation->PerceptionVisualStyleParameterCollection =
		NewObject<UMaterialParameterCollection>(presentation);
	presentation->PerceptionRuntimeParameterCollection =
		NewObject<UMaterialParameterCollection>(presentation);
	TestTrue(TEXT("Presentation defaults"), presentation->Validate(error));
	TestTrue(TEXT("NPC defaults"), FLRNPCTuningSettings().Validate(error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRTuningBoundariesTest, "LostRunic.Tuning.BoundariesAreAccepted",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRTuningBoundariesTest::RunTest(const FString& parameters)
{
	(void)parameters;
	FString error;
	FLRGuardTuningSettings guard;
	guard.SightToChaseGraceSeconds = 0.0f;
	guard.SuspiciousStimulusCooldownSeconds = 0.0f;
	guard.InvestigateStimulusCooldownSeconds = 0.0f;
	guard.FirstAttractRunCooldownMultiplier = 0.0f;
	guard.FirstAttractSneakCooldownMultiplier = 4.0f;
	guard.RoomRunAlertLevel = 1;
	guard.InvestigateMoveRetargetDistanceCm = 1.0f;
	TestTrue(TEXT("Declared Guard boundaries"), guard.Validate(error));

	ULRStateTuning* state = NewObject<ULRStateTuning>();
	state->EnterHoldSeconds = 0.05f;
	state->ExitHoldSeconds = 5.0f;
	state->CourageAttackCooldownSeconds = 0.0f;
	state->CourageKnockbackSpeed = 3000.0f;
	state->CourageAttackRangeCm = 1.0f;
	state->CourageAttackFacingDegrees = 360.0f;
	TestTrue(TEXT("Declared state boundaries"), state->Validate(error));

	ULRSaveTuning* save = NewObject<ULRSaveTuning>();
	save->AutoSaveDebounceSeconds = 0.0f;
	save->RetryCount = 10;
	save->RetryDelaySeconds = 10.0f;
	save->MaxManualSaveSlots = 100;
	TestTrue(TEXT("Declared save boundaries"), save->Validate(error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRTuningInvalidTest, "LostRunic.Tuning.InvalidValuesAreRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRTuningInvalidTest::RunTest(const FString& parameters)
{
	(void)parameters;
	FString error;
	FLRGuardTuningSettings guard;
	guard.RoomRunAlertLevel = 6;
	TestFalse(TEXT("Room run Floor above white band is rejected"), guard.Validate(error));

	guard = FLRGuardTuningSettings();
	guard.InvestigateSpeed = guard.PatrolSpeed - 1.0f;
	TestFalse(TEXT("Investigate speed below patrol speed is rejected"), guard.Validate(error));

	guard = FLRGuardTuningSettings();
	guard.SightTrackingIntervalSeconds = 0.0f;
	TestFalse(TEXT("Zero sight tracking interval is rejected"), guard.Validate(error));

	guard = FLRGuardTuningSettings();
	guard.AttractAlertAmount = 0;
	TestFalse(TEXT("Zero attract amount is rejected"), guard.Validate(error));

	ULRMovementTuning* movement = NewObject<ULRMovementTuning>();
	movement->SneakSpeed = movement->RunSpeed + 1.0f;
	TestFalse(TEXT("Inverted movement speeds"), movement->Validate(error));

	ULRInteractionTuning* interaction = NewObject<ULRInteractionTuning>();
	interaction->ExecuteDistance = interaction->FarHintDistance + 1.0f;
	TestFalse(TEXT("Inverted interaction tiers"), interaction->Validate(error));

	ULRStateTuning* state = NewObject<ULRStateTuning>();
	state->CourageAttackRangeCm = 0.0f;
	TestFalse(TEXT("Attack range below declared minimum"), state->Validate(error));

	ULRUITuning* ui = NewObject<ULRUITuning>();
	ui->TypewriterCharactersPerSecond = 0.0f;
	TestFalse(TEXT("Zero typewriter speed"), ui->Validate(error));
	return true;
}

#endif
