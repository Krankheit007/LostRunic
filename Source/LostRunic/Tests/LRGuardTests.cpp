/**
 * @file LRGuardTests.cpp
 * @brief 提供 LostRunic Runtime 自动化测试，覆盖调优边界、状态矩阵、交互筛选、物品双入口、守卫警戒、叙事分支和存档事务顺序。仅在 WITH_DEV_AUTOMATION_TESTS 下编译。
 *
 * 关联文件：Tests 目录内调用该公共契约的实现文件；所属领域：Tests。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
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
	TestEqual(TEXT("Alert clamps below zero"), LRAlertRules::ApplyDelta(2, -8), 0);
	TestEqual(TEXT("Alert clamps at eleven"), LRAlertRules::ApplyDelta(9, 8), 11);
	TestEqual(TEXT("Zero alert patrols"), LRAlertRules::ResolveState(0, false, false), ELRGuardBehaviorState::IdlePatrol);
	TestEqual(TEXT("Low alert is suspicious"), LRAlertRules::ResolveState(5, false, false), ELRGuardBehaviorState::Suspicious);
	TestEqual(TEXT("Mid alert investigates"), LRAlertRules::ResolveState(6, false, false), ELRGuardBehaviorState::Investigate);
	TestEqual(TEXT("Max alert without sight searches"), LRAlertRules::ResolveState(11, false, false), ELRGuardBehaviorState::Search);
	TestEqual(TEXT("Confirmed max alert chases"), LRAlertRules::ResolveState(11, true, false), ELRGuardBehaviorState::Chase);
	TestEqual(TEXT("Searching in red band searches"), LRAlertRules::ResolveState(6, false, true), ELRGuardBehaviorState::Search);
	TestEqual(TEXT("Searching below red band is suspicious"), LRAlertRules::ResolveState(5, false, true), ELRGuardBehaviorState::Suspicious);
	TestFalse(TEXT("Observation suppresses decay"), LRAlertRules::ShouldDecay(true, false, ELRGuardBehaviorState::Suspicious));
	TestFalse(TEXT("Sight suppresses decay"), LRAlertRules::ShouldDecay(false, true, ELRGuardBehaviorState::Search));
	TestFalse(TEXT("Investigate holds alert while traveling"), LRAlertRules::ShouldDecay(false, false, ELRGuardBehaviorState::Investigate));
	TestFalse(TEXT("Chase holds alert"), LRAlertRules::ShouldDecay(false, false, ELRGuardBehaviorState::Chase));
	TestTrue(TEXT("Suspicious decays after observation"), LRAlertRules::ShouldDecay(false, false, ELRGuardBehaviorState::Suspicious));
	TestTrue(TEXT("Search decays after observation"), LRAlertRules::ShouldDecay(false, false, ELRGuardBehaviorState::Search));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRNoiseAlertDeltaTest, "LostRunic.AI.NoiseAlertDelta",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRNoiseAlertDeltaTest::RunTest(const FString& parameters)
{
	ULRGuardTuning* tuning = NewObject<ULRGuardTuning>(GetTransientPackage());
	if (!TestNotNull(TEXT("Guard tuning created"), tuning))
	{
		return false;
	}

	// 室内奔跑：Set 语义，警戒至少提升到 RoomRunAlertLevel，不走吸引 CD。
	FLRNoiseResponse indoorRun = LRGuardPerceptionRules::ResolveNoiseAlertDelta(
		LRGameplayTags::NoiseFootstepRunIndoor, 3, *tuning);
	TestTrue(TEXT("Indoor run responds"), indoorRun.bRespond);
	TestEqual(TEXT("Indoor run raises to floor"), indoorRun.Delta, 2);
	TestFalse(TEXT("Indoor run is not attract"), indoorRun.bIsAttract);
	indoorRun = LRGuardPerceptionRules::ResolveNoiseAlertDelta(LRGameplayTags::NoiseFootstepRunIndoor, 6, *tuning);
	TestEqual(TEXT("Indoor run above floor is ignored"), indoorRun.Delta, 0);

	// Faint：仅警戒 >=6 的守卫响应，且为吸引语义。
	FLRNoiseResponse faintLow = LRGuardPerceptionRules::ResolveNoiseAlertDelta(
		LRGameplayTags::NoiseFootstepWalkFaint, 5, *tuning);
	TestFalse(TEXT("Faint ignored below six"), faintLow.bRespond);
	TestTrue(TEXT("Faint is attract"), faintLow.bIsAttract);
	FLRNoiseResponse faintHigh = LRGuardPerceptionRules::ResolveNoiseAlertDelta(
		LRGameplayTags::NoiseFootstepWalkFaint, 6, *tuning);
	TestTrue(TEXT("Faint responds at six"), faintHigh.bRespond);
	TestEqual(TEXT("Faint attracts one"), faintHigh.Delta, 1);

	// 普通噪声：一律吸引 +1。
	const FGameplayTag plainReasons[] = {
		LRGameplayTags::NoiseFootstepWalk.GetTag(),
		LRGameplayTags::NoiseFootstepRun.GetTag(),
		LRGameplayTags::NoiseInteraction.GetTag()
	};
	for (const FGameplayTag reason : plainReasons)
	{
		const FLRNoiseResponse response = LRGuardPerceptionRules::ResolveNoiseAlertDelta(reason, 4, *tuning);
		TestTrue(TEXT("Plain noise responds"), response.bRespond);
		TestEqual(TEXT("Plain noise attracts one"), response.Delta, tuning->AttractAlertAmount);
		TestTrue(TEXT("Plain noise is attract"), response.bIsAttract);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRAlertIncreaseCooldownTest, "LostRunic.AI.AlertIncreaseCooldown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRAlertIncreaseCooldownTest::RunTest(const FString& parameters)
{
	ULRGuardTuning* tuning = NewObject<ULRGuardTuning>(GetTransientPackage());
	if (!TestNotNull(TEXT("Guard tuning created"), tuning))
	{
		return false;
	}

	// 1-5 档与首次进入 6-10 档使用 0.5s，6-10 档后续使用 0.2s。
	TestEqual(TEXT("Low band uses long cooldown"),
		LRAlertRules::ResolveAttractIncreaseCooldown(3, false, *tuning), tuning->AlertIncreaseCooldownSeconds);
	TestEqual(TEXT("First increase in red band uses long cooldown"),
		LRAlertRules::ResolveAttractIncreaseCooldown(6, true, *tuning), tuning->AlertIncreaseCooldownSeconds);
	TestEqual(TEXT("Later increases in red band use short cooldown"),
		LRAlertRules::ResolveAttractIncreaseCooldown(6, false, *tuning), tuning->InvestigateIncreaseCooldownSeconds);

	// 冷却边界：等于冷却时长时允许；冷却被拒绝的刺激完全忽略。
	TestTrue(TEXT("Cooldown elapsed allows increase"), LRAlertRules::IsIncreaseAllowed(10.0, 9.5, 0.5f));
	TestFalse(TEXT("Cooldown active rejects increase"), LRAlertRules::IsIncreaseAllowed(10.0, 9.6, 0.5f));
	TestTrue(TEXT("Zero cooldown always allows"), LRAlertRules::IsIncreaseAllowed(10.0, 0.0, 0.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRResolveTargetBehaviorTest, "LostRunic.AI.ResolveTargetBehavior",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRResolveTargetBehaviorTest::RunTest(const FString& parameters)
{
	// 眩晕覆盖一切：感知与警戒继续运行，但行为被钉在 Stunned。
	TestEqual(TEXT("Stun overrides chase"), LRAlertRules::ResolveTargetBehavior(true, 11, true, false),
		ELRGuardBehaviorState::Stunned);
	TestEqual(TEXT("Stun overrides idle"), LRAlertRules::ResolveTargetBehavior(true, 0, false, false),
		ELRGuardBehaviorState::Stunned);
	// 未眩晕时按警戒推导。
	TestEqual(TEXT("Resolved idle"), LRAlertRules::ResolveTargetBehavior(false, 0, false, false),
		ELRGuardBehaviorState::IdlePatrol);
	TestEqual(TEXT("Resolved suspicious"), LRAlertRules::ResolveTargetBehavior(false, 5, false, false),
		ELRGuardBehaviorState::Suspicious);
	TestEqual(TEXT("Resolved investigate"), LRAlertRules::ResolveTargetBehavior(false, 6, false, false),
		ELRGuardBehaviorState::Investigate);
	TestEqual(TEXT("Resolved chase"), LRAlertRules::ResolveTargetBehavior(false, 11, true, false),
		ELRGuardBehaviorState::Chase);
	// 眩晕结束后按当前警戒与视线恢复。
	TestEqual(TEXT("Stun recovery resumes chase"), LRAlertRules::ResolveTargetBehavior(false, 11, true, false),
		ELRGuardBehaviorState::Chase);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRAlertTierTest, "LostRunic.AI.AlertTierMapping",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRAlertTierTest::RunTest(const FString& parameters)
{
	TestEqual(TEXT("Zero alert is hidden"), LRAlertRules::ResolveAlertTier(0), ELRGuardAlertTier::Hidden);
	TestEqual(TEXT("Low alert is white"), LRAlertRules::ResolveAlertTier(1), ELRGuardAlertTier::White);
	TestEqual(TEXT("Five is white boundary"), LRAlertRules::ResolveAlertTier(5), ELRGuardAlertTier::White);
	TestEqual(TEXT("Six is red"), LRAlertRules::ResolveAlertTier(6), ELRGuardAlertTier::Red);
	TestEqual(TEXT("Ten is red boundary"), LRAlertRules::ResolveAlertTier(10), ELRGuardAlertTier::Red);
	TestEqual(TEXT("Eleven is full"), LRAlertRules::ResolveAlertTier(11), ELRGuardAlertTier::Full);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRAlertComponentWorkflowTest, "LostRunic.AI.AlertComponentWorkflow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRAlertComponentWorkflowTest::RunTest(const FString& parameters)
{
	ULRAlertComponent* alert = NewObject<ULRAlertComponent>(GetTransientPackage());
	if (!TestNotNull(TEXT("Alert component created"), alert))
	{
		return false;
	}

	alert->ApplyAlertDelta(6, FVector(100.0f, 0.0f, 0.0f), nullptr, LRGameplayTags::NoiseInteraction);
	TestEqual(TEXT("Alert stimulus enters investigate"), alert->GetBehaviorState(), ELRGuardBehaviorState::Investigate);
	alert->MarkInvestigationReached();
	TestEqual(TEXT("Reached investigation enters search"), alert->GetBehaviorState(), ELRGuardBehaviorState::Search);
	alert->SetSightTarget(nullptr, true, FVector::ZeroVector);
	TestEqual(TEXT("Confirmed sight at max alert enters chase"), alert->GetBehaviorState(), ELRGuardBehaviorState::Chase);
	alert->SetSightTarget(nullptr, false, FVector::ZeroVector);
	TestEqual(TEXT("Lost sight returns to search"), alert->GetBehaviorState(), ELRGuardBehaviorState::Search);
	alert->ResetAfterSearch();
	TestEqual(TEXT("Search reset returns to patrol"), alert->GetBehaviorState(), ELRGuardBehaviorState::IdlePatrol);
	TestEqual(TEXT("Search reset clears alert"), alert->GetAlertLevel(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRGuardPerceptionRulesTest, "LostRunic.AI.PerceptionConeOcclusionAndHearing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRGuardPerceptionRulesTest::RunTest(const FString& parameters)
{
	ULRGuardTuning* tuning = NewObject<ULRGuardTuning>(GetTransientPackage());
	if (!TestNotNull(TEXT("Guard tuning created"), tuning))
	{
		return false;
	}

	const float boundaryDot = FMath::Cos(FMath::DegreesToRadians(tuning->SightConeDegrees * 0.5f));
	TestTrue(TEXT("Sight accepts 500 cm forward target"),
		LRGuardPerceptionRules::CanConfirmSight(500.0f, 1.0f, false, false, *tuning));
	TestTrue(TEXT("Sight accepts cone boundary"),
		LRGuardPerceptionRules::CanConfirmSight(tuning->SightRadius, boundaryDot, false, false, *tuning));
	TestFalse(TEXT("Sight rejects beyond radius"),
		LRGuardPerceptionRules::CanConfirmSight(tuning->SightRadius + 0.1f, 1.0f, false, false, *tuning));
	TestFalse(TEXT("Sight rejects outside cone"),
		LRGuardPerceptionRules::CanConfirmSight(tuning->SightRadius, boundaryDot - 0.01f, false, false, *tuning));
	TestFalse(TEXT("Sight rejects occluded target"),
		LRGuardPerceptionRules::CanConfirmSight(100.0f, 1.0f, true, false, *tuning));
	TestFalse(TEXT("Sight rejects hidden target"),
		LRGuardPerceptionRules::CanConfirmSight(100.0f, 1.0f, false, true, *tuning));
	TestTrue(TEXT("Hearing accepts source radius"), LRGuardPerceptionRules::CanHear(1000.0f, 1000.0f, *tuning));
	TestFalse(TEXT("Hearing rejects outside source radius"), LRGuardPerceptionRules::CanHear(1000.1f, 1000.0f, *tuning));
	tuning->HearingRangeMultiplier = 1.5f;
	TestTrue(TEXT("Hearing multiplier expands radius"), LRGuardPerceptionRules::CanHear(1500.0f, 1000.0f, *tuning));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRGuardCompositionTest, "LostRunic.AI.GuardCompositionDisablesTick",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRGuardCompositionTest::RunTest(const FString& parameters)
{
	TestFalse(TEXT("Guard actor Tick is disabled"), GetDefault<ALRGuardCharacter>()->PrimaryActorTick.bCanEverTick);
	TestFalse(TEXT("Guard controller Tick is disabled"), GetDefault<ALRGuardAIController>()->PrimaryActorTick.bCanEverTick);
	TestFalse(TEXT("Alert component Tick is disabled"), GetDefault<ULRAlertComponent>()->PrimaryComponentTick.bCanEverTick);
	TestFalse(TEXT("Hide component Tick is disabled"), GetDefault<ULRHideComponent>()->PrimaryComponentTick.bCanEverTick);
	TestFalse(TEXT("Noise emitter Tick is disabled"), GetDefault<ULRNoiseEmitterComponent>()->PrimaryComponentTick.bCanEverTick);
	return true;
}

#endif
