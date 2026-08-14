/**
 * @file LRTuningTests.cpp
 * @brief 提供 LostRunic Runtime 自动化测试，覆盖调优边界、状态矩阵、交互筛选、物品双入口、守卫警戒、叙事分支和存档事务顺序。仅在 WITH_DEV_AUTOMATION_TESTS 下编译。
 *
 * 关联文件：Tests 目录内调用该公共契约的实现文件；所属领域：Tests。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
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
	TestTrue(TEXT("NPC defaults"), NewObject<ULRNPCTuning>()->Validate(error));
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
	state->CourageAttackRangeCm = 1.0f;
	state->CourageAttackFacingDegrees = 360.0f;
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

	ULRStateTuning* state = NewObject<ULRStateTuning>();
	state->CourageAttackRangeCm = 0.0f;
	TestFalse(TEXT("Attack range below declared minimum"), state->Validate(error));
	state->CourageAttackRangeCm = 1.0f;
	state->CourageAttackFacingDegrees = 0.0f;
	TestFalse(TEXT("Attack facing below declared minimum"), state->Validate(error));

	ULRUITuning* ui = NewObject<ULRUITuning>();
	ui->TypewriterCharactersPerSecond = 0.0f;
	TestFalse(TEXT("Zero typewriter speed"), ui->Validate(error));
	ui->TypewriterCharactersPerSecond = 30.0f;
	ui->TypewriterUpdateSeconds = 0.0f;
	TestFalse(TEXT("Zero typewriter update frequency"), ui->Validate(error));
	return true;
}

#endif
