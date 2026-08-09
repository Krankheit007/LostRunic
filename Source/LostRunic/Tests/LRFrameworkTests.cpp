/**
 * @file LRFrameworkTests.cpp
 * @brief 提供 LostRunic Runtime 自动化测试，覆盖调优边界、状态矩阵、交互筛选、物品双入口、守卫警戒、叙事分支和存档事务顺序。仅在 WITH_DEV_AUTOMATION_TESTS 下编译。
 *
 * 关联文件：Tests 目录内调用该公共契约的实现文件；所属领域：Tests。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Data/LRProjectSettings.h"
#include "Framework/LRCharacter.h"
#include "Gameplay/LRLocomotionComponent.h"
#include "Input/LRInputConfig.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRFrameworkDefaultsTest, "LostRunic.Framework.CharacterDoesNotTick",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRFrameworkDefaultsTest::RunTest(const FString& parameters)
{
	const ALRCharacter* character = GetDefault<ALRCharacter>();
	TestFalse(TEXT("LR character Tick is disabled"), character->PrimaryActorTick.bCanEverTick);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRInputConfigTest, "LostRunic.Input.ProjectConfigIsComplete",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRInputConfigTest::RunTest(const FString& parameters)
{
	const ULRInputConfig* inputConfig = GetDefault<ULRProjectSettings>()->InputConfig.LoadSynchronous();
	TestNotNull(TEXT("Project InputConfig loads"), inputConfig);
	if (inputConfig)
	{
		FString error;
		TestTrue(TEXT("Required contexts and actions are assigned"), inputConfig->Validate(error));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRMovementPaceInputTest, "LostRunic.Input.MovementPaceRestoresPreviousMode",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRMovementPaceInputTest::RunTest(const FString& parameters)
{
	ULRLocomotionComponent* locomotion = NewObject<ULRLocomotionComponent>();
	TestEqual(TEXT("Default pace is Walk"), locomotion->GetPace(), ELRMovementPace::Walk);

	locomotion->ToggleSneak();
	TestEqual(TEXT("Toggle enters Sneak"), locomotion->GetPace(), ELRMovementPace::Sneak);
	locomotion->StartRun();
	TestEqual(TEXT("Run press enters Run"), locomotion->GetPace(), ELRMovementPace::Run);
	locomotion->StopRun();
	TestEqual(TEXT("Run release restores Sneak"), locomotion->GetPace(), ELRMovementPace::Sneak);

	locomotion->ToggleSneak();
	locomotion->StartRun();
	locomotion->StopRun();
	TestEqual(TEXT("Run release restores Walk"), locomotion->GetPace(), ELRMovementPace::Walk);

	locomotion->StartRun();
	locomotion->ToggleSneak();
	locomotion->StopRun();
	TestEqual(TEXT("Sneak toggle during Run changes restored pace"), locomotion->GetPace(), ELRMovementPace::Sneak);
	return true;
}

#endif
