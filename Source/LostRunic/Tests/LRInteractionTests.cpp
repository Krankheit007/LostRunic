#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Data/LRInteractionTuning.h"
#include "Interaction/LRInteractionRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRInteractionCandidateTest, "LostRunic.Interaction.SelectsNearestLegalCandidate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRInteractionCandidateTest::RunTest(const FString& parameters)
{
	const ULRInteractionTuning* tuning = GetDefault<ULRInteractionTuning>();
	TArray<FLRInteractionCandidateScore> candidates;
	candidates.Add({ 180.0f, 1.0f, true, true, true });
	candidates.Add({ 350.0f, 1.0f, false, true, true });
	candidates.Add({ 120.0f, 0.0f, false, true, true });
	candidates.Add({ 90.0f, 1.0f, false, false, true });
	candidates.Add({ 70.0f, 1.0f, false, true, false });

	TestEqual(TEXT("Nearest candidate surviving occlusion, facing, state, and item rules is selected"),
		LRInteractionRules::SelectBestCandidate(candidates, *tuning), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRInteractionRangeTest, "LostRunic.Interaction.DistanceBandsAndFacingBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRInteractionRangeTest::RunTest(const FString& parameters)
{
	const ULRInteractionTuning* tuning = GetDefault<ULRInteractionTuning>();
	TestEqual(TEXT("200 cm is executable"), LRInteractionRules::GetRange(200.0f, 200.0f, *tuning),
		ELRInteractionRange::Executable);
	TestEqual(TEXT("500 cm is outlined"), LRInteractionRules::GetRange(500.0f, 200.0f, *tuning),
		ELRInteractionRange::Outline);
	TestEqual(TEXT("1000 cm has a far hint"), LRInteractionRules::GetRange(1000.0f, 200.0f, *tuning),
		ELRInteractionRange::FarHint);
	TestEqual(TEXT("Beyond the far hint is hidden"), LRInteractionRules::GetRange(1000.1f, 200.0f, *tuning),
		ELRInteractionRange::None);

	const float boundaryDot = FMath::Cos(FMath::DegreesToRadians(tuning->FacingConeDegrees * 0.5f));
	TestTrue(TEXT("Total 90 degree cone includes its 45 degree half-angle boundary"),
		LRInteractionRules::IsFacingAllowed(boundaryDot, *tuning));
	TestFalse(TEXT("Target outside the facing cone is rejected"),
		LRInteractionRules::IsFacingAllowed(boundaryDot - 0.01f, *tuning));
	return true;
}

#endif
