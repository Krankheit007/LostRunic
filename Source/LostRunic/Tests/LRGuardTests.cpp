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
	TestEqual(TEXT("Max alert searches after sight is lost"), LRAlertRules::ResolveState(11, false, false), ELRGuardBehaviorState::Search);
	TestEqual(TEXT("Confirmed max alert chases"), LRAlertRules::ResolveState(11, true, false), ELRGuardBehaviorState::Chase);
	TestFalse(TEXT("Sight suppresses decay"), LRAlertRules::ShouldDecay(30.0f, 3.0f, true));
	TestFalse(TEXT("Observation delay has not elapsed"), LRAlertRules::ShouldDecay(2.99f, 3.0f, false));
	TestTrue(TEXT("Observation delay boundary decays"), LRAlertRules::ShouldDecay(3.0f, 3.0f, false));
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
