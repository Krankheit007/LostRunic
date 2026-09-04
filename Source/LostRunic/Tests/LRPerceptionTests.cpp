/**
 * @file LRPerceptionTests.cpp
 * @brief Focused tests for Perception timing, fixed-slot allocation and cross-asset invariants.
 */
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Data/LRGameContentSet.h"
#include "Data/LRGameTuningSet.h"
#include "Data/LRInteractionTuning.h"
#include "Data/LRMovementTuning.h"
#include "Data/LRPresentationTuning.h"
#include "Data/LRSaveTuning.h"
#include "Data/LRStateTuning.h"
#include "Data/LRUITuning.h"
#include "Data/LRVisualStyleDefinition.h"
#include "Perception/LRPerceptionRules.h"
#include "Perception/LRPerceptionSoundSourceComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRPerceptionEchoRulesTest, "LostRunic.Perception.EchoRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRPerceptionEchoRulesTest::RunTest(const FString& parameters)
{
	(void)parameters;
	const float now = 10.0f;
	UObject* source = NewObject<ULRVisualStyleDefinition>();
	FLRPerceptionEchoSlot slot;
	slot.bActive = true;
	slot.Center = FVector::ZeroVector;
	slot.ExpireTime = now + 1.0f;
	slot.SourceObject = source;

	FLRPerceptionPulseRequest request;
	request.SourceObject = source;
	request.WorldLocation = FVector(0.0f, 0.0f, 24.0f);
	request.bRefreshExistingSource = true;
	TestTrue(TEXT("Nearby same-source looping pulse refreshes"),
		LRPerceptionRules::CanRefreshSlot(slot, request, now, 25.0f));

	request.WorldLocation = FVector(0.0f, 0.0f, 26.0f);
	TestFalse(TEXT("Spatially separated pulse creates a new event"),
		LRPerceptionRules::CanRefreshSlot(slot, request, now, 25.0f));
	request.bRefreshExistingSource = false;
	TestFalse(TEXT("One-shot pulse never refreshes"),
		LRPerceptionRules::CanRefreshSlot(slot, request, now, 25.0f));

	request.WorldLocation = FVector::ZeroVector;
	request.bRefreshExistingSource = true;
	slot.ExpireTime = now;
	TestFalse(TEXT("Expired source slot cannot refresh"),
		LRPerceptionRules::CanRefreshSlot(slot, request, now, 25.0f));

	TestEqual(TEXT("Residue is fully wet before dry interval"),
		LRPerceptionRules::ComputeResidueFade(1.3f, 1.3f), 1.0f, 0.001f);
	TestEqual(TEXT("Residue linearly washes during dry interval"),
		LRPerceptionRules::ComputeResidueFade(0.65f, 1.3f), 0.5f, 0.001f);
	TestEqual(TEXT("Residue is gone at expiry"),
		LRPerceptionRules::ComputeResidueFade(0.0f, 1.3f), 0.0f, 0.001f);

	TArray<FLRPerceptionEchoSlot> slots;
	slots.SetNum(8);
	TestEqual(TEXT("First empty slot is selected"), LRPerceptionRules::SelectSlot(slots, now), 0);
	for (int32 index = 0; index < slots.Num(); ++index)
	{
		slots[index].bActive = true;
		slots[index].ExpireTime = now + 5.0f + index;
	}
	TestEqual(TEXT("Earliest expiry is selected when all eight slots are active"),
		LRPerceptionRules::SelectSlot(slots, now), 0);
	slots[3].ExpireTime = now - 1.0f;
	TestEqual(TEXT("An expired slot is preferred before eviction"),
		LRPerceptionRules::SelectSlot(slots, now), 3);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRPerceptionTuningContractTest, "LostRunic.Perception.TuningContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRPerceptionTuningContractTest::RunTest(const FString& parameters)
{
	(void)parameters;
	ULRGameTuningSet* tuningSet = NewObject<ULRGameTuningSet>();
	tuningSet->State = NewObject<ULRStateTuning>(tuningSet);
	tuningSet->Movement = NewObject<ULRMovementTuning>(tuningSet);
	tuningSet->Interaction = NewObject<ULRInteractionTuning>(tuningSet);
	tuningSet->Save = NewObject<ULRSaveTuning>(tuningSet);
	tuningSet->UI = NewObject<ULRUITuning>(tuningSet);
	tuningSet->Presentation = NewObject<ULRPresentationTuning>(tuningSet);

	FString error;
	AddInfo(FString::Printf(TEXT("Presentation CDO CutawayRadiusRefPx=%.3f; fixture CutawayRadiusRefPx=%.3f; NoiseRevealDurationSeconds=%.3f; EchoDryFadeDurationSeconds=%.3f."),
		GetDefault<ULRPresentationTuning>()->CutawayRadiusRefPx,
		tuningSet->Presentation->CutawayRadiusRefPx,
		tuningSet->Presentation->NoiseRevealDurationSeconds,
		tuningSet->Presentation->EchoDryFadeDurationSeconds));
	FString presentationError;
	const bool bPresentationValid = tuningSet->Presentation->Validate(presentationError);
	if (!bPresentationValid)
	{
		AddError(FString::Printf(TEXT("Direct Presentation validation failed: %s"), *presentationError));
	}
	TestTrue(TEXT("Presentation fixture validates directly"), bPresentationValid);
	const bool bDefaultsValid = tuningSet->Validate(error);
	if (!bDefaultsValid)
	{
		AddError(error);
	}
	TestTrue(TEXT("Default cross-system tuning is valid"), bDefaultsValid);

	tuningSet->Presentation->PerceptionFullRevealRadius =
		tuningSet->Presentation->PerceptionRevealRadius + 1.0f;
	TestFalse(TEXT("Full reveal cannot exceed player reveal radius"), tuningSet->Validate(error));
	tuningSet->Presentation->PerceptionFullRevealRadius = 400.0f;

	tuningSet->Presentation->EchoWetSeconds = 0.0f;
	TestFalse(TEXT("Wet interval must be positive"), tuningSet->Validate(error));
	tuningSet->Presentation->EchoWetSeconds = 0.20f;

	tuningSet->Interaction->ExecuteDistance = 401.0f;
	TestFalse(TEXT("Interaction execute distance cannot exceed full reveal"), tuningSet->Validate(error));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRPerceptionVisualStyleResolutionTest,
	"LostRunic.Perception.VisualStyleResolution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRPerceptionVisualStyleResolutionTest::RunTest(const FString& parameters)
{
	(void)parameters;
	ULRGameContentSet* contentSet = NewObject<ULRGameContentSet>();
	ULRVisualStyleDefinition* defaultStyle = NewObject<ULRVisualStyleDefinition>(contentSet);
	ULRVisualStyleDefinition* mapStyle = NewObject<ULRVisualStyleDefinition>(contentSet);
	contentSet->DefaultVisualStyle = defaultStyle;

	FLRMapRegistration& map = contentSet->Maps.AddDefaulted_GetRef();
	map.MapId = FName(TEXT("TestMap"));
	map.VisualStyleOverride = TSoftObjectPtr<ULRVisualStyleDefinition>(mapStyle);

	TestTrue(TEXT("Project default style resolves without a map override"),
		contentSet->ResolveVisualStyle(NAME_None) == defaultStyle);
	TestTrue(TEXT("Map override resolves before the project default"),
		contentSet->ResolveVisualStyle(FName(TEXT("TestMap"))) == mapStyle);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRPerceptionSoundSourceContractTest,
	"LostRunic.Perception.SoundSourceContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRPerceptionSoundSourceContractTest::RunTest(const FString& parameters)
{
	(void)parameters;
	const ULRPerceptionSoundSourceComponent* source = NewObject<ULRPerceptionSoundSourceComponent>();
	TestFalse(TEXT("Ambient source does not report to AI by default"), source->bAlsoEmitToAI);
	TestEqual(TEXT("Ambient visual radius remains tuning-resolved by default"),
		source->VisualRadiusOverrideCm, 0.0f, 0.001f);
	TestEqual(TEXT("Ambient AI radius is independent and disabled by default"),
		source->AIHearingRadiusCm, 0.0f, 0.001f);
	return true;
}

#endif
