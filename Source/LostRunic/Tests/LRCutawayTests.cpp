#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Camera/LRCutawayTargetComponent.h"
#include "Camera/LRCutawayTypes.h"
#include "Camera/LRCameraCutawayComponent.h"
#include "Core/LRCustomPrimitiveData.h"
#include "Core/LRCustomStencil.h"
#include "Data/LRPresentationTuning.h"
#include "Math/UnrealMathUtility.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	ULRCutawayTargetComponent* MakeStickyTestTarget(UObject* outer, const float amount)
	{
		ULRCutawayTargetComponent* target = NewObject<ULRCutawayTargetComponent>(outer);
		target->SetCutawayRequest(target, ELRCutawayRequestType::Local, amount, true);
		return target;
	}

	float ExpectedActiveCountGate(const int32 activeCount, const int32 slotIndex)
	{
		return FMath::Clamp(static_cast<float>(activeCount - slotIndex), 0.0f, 1.0f);
	}

	float SmoothStep01(const float edge0, const float edge1, const float value)
	{
		const float t = FMath::Clamp((value - edge0) / (edge1 - edge0), 0.0f, 1.0f);
		return t * t * (3.0f - 2.0f * t);
	}

	float ExpectedFiniteSuppressionRing(const float signedDistance, const float transitionRefPx)
	{
		constexpr float LowerMarginRefPx = 2.0f;
		constexpr float UpperBoundaryRefPx = 2.0f;
		constexpr float FalloffRefPx = 1.0f;
		const float lowerBoundary = -transitionRefPx - LowerMarginRefPx;
		const float lowerRamp = SmoothStep01(
			lowerBoundary - FalloffRefPx, lowerBoundary, signedDistance);
		const float upperRamp = SmoothStep01(
			UpperBoundaryRefPx, UpperBoundaryRefPx + FalloffRefPx, signedDistance);
		return lowerRamp * (1.0f - upperRamp);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRCutawayTransitionTest, "LostRunic.Cutaway.TransitionReversesFromCurrentValue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRCutawayTransitionTest::RunTest(const FString& parameters)
{
	(void)parameters;
	FLRCutawayChannelState state;
	state.Retarget(1.0f, 0.0, 0.25f, 0.35f);
	state.Evaluate(0.15);
	const float beforeReverse = state.CurrentAmount;
	TestTrue(TEXT("Partial hide reached an intermediate amount"), beforeReverse > 0.0f && beforeReverse < 1.0f);

	state.Retarget(0.0f, 0.15, 0.25f, 0.35f);
	TestEqual(TEXT("Retarget does not jump"), state.CurrentAmount, beforeReverse, 0.001f);
	TestEqual(TEXT("Transition starts from sampled current value"), state.TransitionStartAmount, beforeReverse, 0.001f);
	state.Evaluate(1.0);
	TestEqual(TEXT("Restore reaches zero"), state.CurrentAmount, 0.0f, 0.001f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRCutawayContractTest, "LostRunic.Cutaway.ContractsAreStable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRCutawayContractTest::RunTest(const FString& parameters)
{
	(void)parameters;
	TestEqual(TEXT("CPD contract contains nine floats"), LRCustomPrimitiveData::Count, 9);
	TestEqual(TEXT("Player occlusion stencil is centrally reserved"), LRCustomStencil::PlayerOccluded, static_cast<uint8>(2));
	const ULRPresentationTuning* tuning = GetDefault<ULRPresentationTuning>();
	TestEqual(TEXT("Local radius reference"), tuning->CutawayRadiusRefPx, 200.0f, 0.001f);
	TestEqual(TEXT("Detection frequency"), tuning->CutawayDetectionFrequencyHz, 10.0f, 0.001f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRCutawayFiniteRingMathTest,
	"LostRunic.Cutaway.FiniteSuppressionRing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRCutawayFiniteRingMathTest::RunTest(const FString& parameters)
{
	(void)parameters;
	constexpr float transitionRefPx = 8.0f;
	constexpr float lowerBoundary = -10.0f;
	constexpr float upperBoundary = 2.0f;

	TestEqual(TEXT("Deep cutaway interior is not suppressed"),
		ExpectedFiniteSuppressionRing(lowerBoundary - 2.0f, transitionRefPx), 0.0f, 0.001f);
	TestEqual(TEXT("Lower boundary falloff is half strength"),
		ExpectedFiniteSuppressionRing(lowerBoundary - 0.5f, transitionRefPx), 0.5f, 0.001f);
	TestEqual(TEXT("Finite band interior is fully suppressed"),
		ExpectedFiniteSuppressionRing(0.0f, transitionRefPx), 1.0f, 0.001f);
	TestEqual(TEXT("Upper boundary remains fully suppressed"),
		ExpectedFiniteSuppressionRing(upperBoundary, transitionRefPx), 1.0f, 0.001f);
	TestEqual(TEXT("Upper boundary falloff is half strength"),
		ExpectedFiniteSuppressionRing(upperBoundary + 0.5f, transitionRefPx), 0.5f, 0.001f);
	TestEqual(TEXT("Far exterior is not suppressed"),
		ExpectedFiniteSuppressionRing(upperBoundary + 2.0f, transitionRefPx), 0.0f, 0.001f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRCutawayRequestAggregationTest,
	"LostRunic.Cutaway.RequestsUseMaximumAndZeroClears",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRCutawayRequestAggregationTest::RunTest(const FString& parameters)
{
	(void)parameters;
	ULRCutawayTargetComponent* target = NewObject<ULRCutawayTargetComponent>();
	UObject* firstRequester = NewObject<ULRCutawayTargetComponent>(target);
	UObject* secondRequester = NewObject<ULRCutawayTargetComponent>(target);

	TestTrue(TEXT("First local request accepted"),
		target->SetCutawayRequest(firstRequester, ELRCutawayRequestType::Local, 0.4f, true));
	TestTrue(TEXT("Second local request accepted"),
		target->SetCutawayRequest(secondRequester, ELRCutawayRequestType::Local, 0.8f, true));
	TestEqual(TEXT("Channel uses maximum request"),
		target->GetCurrentCutawayAmount(ELRCutawayRequestType::Local), 0.8f, 0.001f);

	target->SetCutawayRequest(secondRequester, ELRCutawayRequestType::Local, 0.0f, true);
	TestEqual(TEXT("Zero amount clears only that requester"),
		target->GetCurrentCutawayAmount(ELRCutawayRequestType::Local), 0.4f, 0.001f);
	target->ClearCutawayRequest(firstRequester, ELRCutawayRequestType::Local, true);
	TestEqual(TEXT("Clearing final requester restores zero"),
		target->GetCurrentCutawayAmount(ELRCutawayRequestType::Local), 0.0f, 0.001f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRCutawayStickyPriorityTest,
	"LostRunic.Cutaway.StickySlotsPrioritizeActiveAmountDistanceAndKey",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRCutawayStickyPriorityTest::RunTest(const FString& parameters)
{
	(void)parameters;
	LR::Cutaway::FStickyCandidate recovering;
	recovering.bActive = false;
	recovering.Amount = 1.0f;
	recovering.DistanceSquared = 1.0f;
	LR::Cutaway::FStickyCandidate active = recovering;
	active.bActive = true;
	TestTrue(TEXT("Active candidate wins over recovering candidate"),
		LR::Cutaway::IsStickyCandidateHigherPriority(active, recovering));

	recovering.bActive = true;
	active.Amount = 0.8f;
	recovering.Amount = 0.4f;
	TestTrue(TEXT("Higher current amount wins among active candidates"),
		LR::Cutaway::IsStickyCandidateHigherPriority(active, recovering));

	active.Amount = recovering.Amount;
	active.DistanceSquared = 4.0f;
	recovering.DistanceSquared = 9.0f;
	TestTrue(TEXT("Nearer candidate wins after amount"),
		LR::Cutaway::IsStickyCandidateHigherPriority(active, recovering));

	UObject* leftObject = NewObject<ULRCutawayTargetComponent>();
	UObject* rightObject = NewObject<ULRCutawayTargetComponent>();
	LR::Cutaway::FStickyCandidate leftKey = active;
	LR::Cutaway::FStickyCandidate rightKey = active;
	leftKey.ObjectKey = FObjectKey(leftObject);
	rightKey.ObjectKey = FObjectKey(rightObject);
	TestTrue(TEXT("Object key tie-break is deterministic"),
		LR::Cutaway::IsStickyCandidateHigherPriority(leftKey, rightKey)
			!= LR::Cutaway::IsStickyCandidateHigherPriority(rightKey, leftKey));

	for (int32 activeCount = 0; activeCount <= 4; ++activeCount)
	{
		for (int32 slotIndex = 0; slotIndex < 4; ++slotIndex)
		{
			const float actual = ExpectedActiveCountGate(activeCount, slotIndex);
			const float expected = (slotIndex < activeCount) ? 1.0f : 0.0f;
			TestEqual(FString::Printf(TEXT("ActiveCount %d slot %d gate"), activeCount, slotIndex),
				actual, expected, 0.001f);
		}
	}

	ULRCameraCutawayComponent* component = NewObject<ULRCameraCutawayComponent>();
	ULRCutawayTargetComponent* recoveringTarget = MakeStickyTestTarget(component, 0.5f);
	component->StickySlotTargets[0] = recoveringTarget;
	component->StickySlotAmounts[0] = 0.5f;
	component->UpdatingTargets.Add(recoveringTarget);
	for (int32 slotIndex = 1; slotIndex < 4; ++slotIndex)
	{
		ULRCutawayTargetComponent* activeTarget = MakeStickyTestTarget(component, 0.4f + slotIndex * 0.1f);
		component->StickySlotTargets[slotIndex] = activeTarget;
		component->StickySlotAmounts[slotIndex] = activeTarget->GetCurrentCutawayAmount(ELRCutawayRequestType::Local);
		component->UpdatingTargets.Add(activeTarget);
		component->RequestedTargets.Add(activeTarget);
	}
	ULRCutawayTargetComponent* blockerTarget = MakeStickyTestTarget(component, 1.0f);
	component->UpdatingTargets.Add(blockerTarget);
	component->RequestedTargets.Add(blockerTarget);
	component->UpdateStickySlots();
	TestTrue(TEXT("Full sticky slots do not evict a recovering member"),
		component->IsStickyTarget(recoveringTarget));
	TestFalse(TEXT("Full sticky slots leave the new blocker waiting"),
		component->IsStickyTarget(blockerTarget));

	recoveringTarget->SetCutawayRequest(recoveringTarget, ELRCutawayRequestType::Local, 0.02f, true);
	component->RefreshStickySlotAmounts();
	TestFalse(TEXT("Non-active sticky member releases at threshold"),
		component->IsStickyTarget(recoveringTarget));
	component->RequestedTargets.Remove(blockerTarget);
	component->UpdatingTargets.Remove(blockerTarget);
	blockerTarget->SetCutawayRequest(blockerTarget, ELRCutawayRequestType::Local, 0.0f, true);

	ULRCutawayTargetComponent* thresholdTarget = MakeStickyTestTarget(component, 0.02f);
	component->UpdatingTargets.Add(thresholdTarget);
	component->UpdateStickySlots();
	TestFalse(TEXT("Non-active threshold target is not admitted"),
		component->IsStickyTarget(thresholdTarget));

	ULRCutawayTargetComponent* waitingRecoveringTarget = MakeStickyTestTarget(component, 0.6f);
	component->UpdatingTargets.Add(waitingRecoveringTarget);
	component->UpdateStickySlots();
	TestTrue(TEXT("Updating recovering target fills a released slot"),
		component->IsStickyTarget(waitingRecoveringTarget));
	return true;
}

#endif
