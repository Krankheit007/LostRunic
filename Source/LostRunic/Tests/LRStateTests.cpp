#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/LRGameplayTags.h"
#include "Data/LRStateTuning.h"
#include "State/LRStateComponent.h"
#include "State/LRStateRules.h"

namespace
{
	FLRStateChangeRequest MakeRequest(const ELRPerceptionMode targetMode, const ELRStateRequestType requestType)
	{
		FLRStateChangeRequest request;
		request.TargetMode = targetMode;
		request.RequestType = requestType;
		request.Source = LRStateRules::GetSourceTag(requestType);
		return request;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRStateTransitionMatrixTest, "LostRunic.State.TransitionMatrix",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRStateTransitionMatrixTest::RunTest(const FString& parameters)
{
	TestTrue(TEXT("Normal closes eyes into Perception"), LRStateRules::IsTransitionAllowed(ELRPerceptionMode::Normal,
		MakeRequest(ELRPerceptionMode::Perception, ELRStateRequestType::CloseEyes)));
	TestTrue(TEXT("Perception opens eyes into Normal"), LRStateRules::IsTransitionAllowed(ELRPerceptionMode::Perception,
		MakeRequest(ELRPerceptionMode::Normal, ELRStateRequestType::OpenEyes)));
	TestTrue(TEXT("Normal opens eyes into Courage"), LRStateRules::IsTransitionAllowed(ELRPerceptionMode::Normal,
		MakeRequest(ELRPerceptionMode::Courage, ELRStateRequestType::OpenEyes)));
	TestTrue(TEXT("Courage closes eyes into Normal"), LRStateRules::IsTransitionAllowed(ELRPerceptionMode::Courage,
		MakeRequest(ELRPerceptionMode::Normal, ELRStateRequestType::CloseEyes)));

	for (const ELRPerceptionMode playableMode : { ELRPerceptionMode::Normal, ELRPerceptionMode::Perception, ELRPerceptionMode::Courage })
	{
		TestTrue(TEXT("Death enters Memory from every playable state"), LRStateRules::IsTransitionAllowed(playableMode,
			MakeRequest(ELRPerceptionMode::Memory, ELRStateRequestType::Death)));
	}
	TestFalse(TEXT("Eye input cannot enter Memory"), LRStateRules::IsTransitionAllowed(ELRPerceptionMode::Normal,
		MakeRequest(ELRPerceptionMode::Memory, ELRStateRequestType::CloseEyes)));
	TestFalse(TEXT("Courage cannot return through OpenEyes"), LRStateRules::IsTransitionAllowed(ELRPerceptionMode::Courage,
		MakeRequest(ELRPerceptionMode::Normal, ELRStateRequestType::OpenEyes)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRStateInputGateTest, "LostRunic.State.InputGateFirstPressWins",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRStateInputGateTest::RunTest(const FString& parameters)
{
	FLRStateInputGate gate;
	TestTrue(TEXT("First input owns the gate"), gate.Press(ELRStateRequestType::CloseEyes));
	TestFalse(TEXT("Second input is rejected"), gate.Press(ELRStateRequestType::OpenEyes));
	TestTrue(TEXT("Early owner release cancels"), gate.Release(ELRStateRequestType::CloseEyes));
	TestFalse(TEXT("Held second input cannot inherit ownership"), gate.Press(ELRStateRequestType::OpenEyes));
	gate.Release(ELRStateRequestType::OpenEyes);
	TestTrue(TEXT("A fresh press starts after all release"), gate.Press(ELRStateRequestType::OpenEyes));
	TestTrue(TEXT("Threshold submits once"), gate.ConsumeThreshold(ELRStateRequestType::OpenEyes));
	TestFalse(TEXT("Threshold cannot submit twice"), gate.ConsumeThreshold(ELRStateRequestType::OpenEyes));
	TestFalse(TEXT("Release after threshold is not cancellation"), gate.Release(ELRStateRequestType::OpenEyes));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRStateHoldAndPresentationTest, "LostRunic.State.HoldDurationsAndPresentationLock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRStateHoldAndPresentationTest::RunTest(const FString& parameters)
{
	const ULRStateTuning* tuning = GetDefault<ULRStateTuning>();
	ELRPerceptionMode targetMode = ELRPerceptionMode::Normal;
	float holdSeconds = 0.0f;
	TestTrue(TEXT("Normal CloseEyes resolves"), LRStateRules::ResolveEyeTransition(ELRPerceptionMode::Normal,
		ELRStateRequestType::CloseEyes, *tuning, targetMode, holdSeconds));
	TestEqual(TEXT("Entering uses enter hold"), holdSeconds, 0.8f);
	TestTrue(TEXT("Perception OpenEyes resolves"), LRStateRules::ResolveEyeTransition(ELRPerceptionMode::Perception,
		ELRStateRequestType::OpenEyes, *tuning, targetMode, holdSeconds));
	TestEqual(TEXT("Returning uses exit hold"), holdSeconds, 0.3f);

	ULRStateComponent* state = NewObject<ULRStateComponent>();
	const FLRStateChangeResult enterResult = state->RequestStateChange(
		MakeRequest(ELRPerceptionMode::Perception, ELRStateRequestType::CloseEyes));
	TestTrue(TEXT("Legal request is accepted"), enterResult.bAccepted);
	TestTrue(TEXT("Accepted request locks presentation"), state->IsPresentationLocked());
	const FLRStateChangeResult lockedResult = state->RequestStateChange(
		MakeRequest(ELRPerceptionMode::Normal, ELRStateRequestType::OpenEyes));
	TestFalse(TEXT("Request during presentation is rejected"), lockedResult.bAccepted);
	TestTrue(TEXT("Presentation rejection is structured"),
		lockedResult.Reason == LRGameplayTags::StateRejectPresentationLocked);
	state->NotifyPresentationComplete();
	TestTrue(TEXT("Completion releases presentation lock"), state->RequestStateChange(
		MakeRequest(ELRPerceptionMode::Normal, ELRStateRequestType::OpenEyes)).bAccepted);

	const FLRStateChangeResult deathResult = state->RequestStateChange(
		MakeRequest(ELRPerceptionMode::Memory, ELRStateRequestType::Death));
	TestTrue(TEXT("Death overrides an active presentation lock"), deathResult.bAccepted);
	return true;
}

#endif
