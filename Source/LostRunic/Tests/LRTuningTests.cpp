#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Data/LRGuardTuning.h"
#include "Data/LRInteractionTuning.h"
#include "Data/LRMovementTuning.h"
#include "Data/LRPresentationTuning.h"
#include "Data/LRSaveTuning.h"
#include "Data/LRStateTuning.h"
#include "Data/LRUITuning.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRTuningDefaultsTest, "LostRunic.Tuning.DefaultsAreValid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRTuningDefaultsTest::RunTest(const FString& parameters)
{
	FString error;
	TestTrue(TEXT("State defaults"), NewObject<ULRStateTuning>()->Validate(error));
	TestTrue(TEXT("Movement defaults"), NewObject<ULRMovementTuning>()->Validate(error));
	TestTrue(TEXT("Interaction defaults"), NewObject<ULRInteractionTuning>()->Validate(error));
	TestTrue(TEXT("Guard defaults"), NewObject<ULRGuardTuning>()->Validate(error));
	TestTrue(TEXT("Save defaults"), NewObject<ULRSaveTuning>()->Validate(error));
	TestTrue(TEXT("UI defaults"), NewObject<ULRUITuning>()->Validate(error));
	TestTrue(TEXT("Presentation defaults"), NewObject<ULRPresentationTuning>()->Validate(error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRTuningBoundariesTest, "LostRunic.Tuning.BoundariesAreAccepted",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRTuningBoundariesTest::RunTest(const FString& parameters)
{
	ULRStateTuning* state = NewObject<ULRStateTuning>();
	state->EnterHoldSeconds = 0.05f;
	state->ExitHoldSeconds = 5.0f;
	state->CourageAttackCooldownSeconds = 0.0f;
	state->CourageKnockbackSpeed = 3000.0f;
	FString error;
	TestTrue(TEXT("Declared state boundaries"), state->Validate(error));

	ULRSaveTuning* save = NewObject<ULRSaveTuning>();
	save->AutoSaveDebounceSeconds = 0.0f;
	save->RetryCount = 10;
	save->RetryDelaySeconds = 10.0f;
	save->ManualSlotCount = 100;
	TestTrue(TEXT("Declared save boundaries"), save->Validate(error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRTuningInvalidTest, "LostRunic.Tuning.InvalidValuesAreRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRTuningInvalidTest::RunTest(const FString& parameters)
{
	FString error;
	ULRMovementTuning* movement = NewObject<ULRMovementTuning>();
	movement->SneakSpeed = movement->RunSpeed + 1.0f;
	TestFalse(TEXT("Inverted movement speeds"), movement->Validate(error));

	ULRInteractionTuning* interaction = NewObject<ULRInteractionTuning>();
	interaction->ExecuteDistance = interaction->FarHintDistance + 1.0f;
	TestFalse(TEXT("Inverted interaction tiers"), interaction->Validate(error));

	ULRGuardTuning* guard = NewObject<ULRGuardTuning>();
	guard->SightConeDegrees = 181.0f;
	TestFalse(TEXT("Sight cone above declared maximum"), guard->Validate(error));

	ULRUITuning* ui = NewObject<ULRUITuning>();
	ui->TypewriterCharactersPerSecond = 0.0f;
	TestFalse(TEXT("Zero typewriter speed"), ui->Validate(error));
	ui->TypewriterCharactersPerSecond = 30.0f;
	ui->TypewriterUpdateSeconds = 0.0f;
	TestFalse(TEXT("Zero typewriter update frequency"), ui->Validate(error));
	return true;
}

#endif
