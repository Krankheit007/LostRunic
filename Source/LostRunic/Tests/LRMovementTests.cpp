/**
 * @file LRMovementTests.cpp
 * @brief 提供移动纯规则自动化测试：状态×步态矩阵、默认步态、步态×环境脚步噪声、噪声环境优先级与室内奔跑房间警戒目标值。仅在 WITH_DEV_AUTOMATION_TESTS 下编译。
 *
 * 关联文件：Tests 目录内调用该公共契约的实现文件；所属领域：Tests。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/LRGameplayTags.h"
#include "Core/LRTypes.h"
#include "Data/LRGuardTuning.h"
#include "Data/LRMovementTuning.h"
#include "Gameplay/LRMovementRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRPaceRulesTest, "LostRunic.Movement.PaceRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRPaceRulesTest::RunTest(const FString& parameters)
{
	// Normal：全部步态。
	TestTrue(TEXT("Normal allows sneak"), LRMovementRules::IsPaceAllowed(ELRPerceptionMode::Normal, ELRMovementPace::Sneak));
	TestTrue(TEXT("Normal allows walk"), LRMovementRules::IsPaceAllowed(ELRPerceptionMode::Normal, ELRMovementPace::Walk));
	TestTrue(TEXT("Normal allows run"), LRMovementRules::IsPaceAllowed(ELRPerceptionMode::Normal, ELRMovementPace::Run));
	// Perception：仅潜行。
	TestTrue(TEXT("Perception allows sneak"), LRMovementRules::IsPaceAllowed(ELRPerceptionMode::Perception, ELRMovementPace::Sneak));
	TestFalse(TEXT("Perception forbids walk"), LRMovementRules::IsPaceAllowed(ELRPerceptionMode::Perception, ELRMovementPace::Walk));
	TestFalse(TEXT("Perception forbids run"), LRMovementRules::IsPaceAllowed(ELRPerceptionMode::Perception, ELRMovementPace::Run));
	// Courage：走路+奔跑。
	TestFalse(TEXT("Courage forbids sneak"), LRMovementRules::IsPaceAllowed(ELRPerceptionMode::Courage, ELRMovementPace::Sneak));
	TestTrue(TEXT("Courage allows walk"), LRMovementRules::IsPaceAllowed(ELRPerceptionMode::Courage, ELRMovementPace::Walk));
	TestTrue(TEXT("Courage allows run"), LRMovementRules::IsPaceAllowed(ELRPerceptionMode::Courage, ELRMovementPace::Run));
	// Memory：仅走路。
	TestFalse(TEXT("Memory forbids sneak"), LRMovementRules::IsPaceAllowed(ELRPerceptionMode::Memory, ELRMovementPace::Sneak));
	TestTrue(TEXT("Memory allows walk"), LRMovementRules::IsPaceAllowed(ELRPerceptionMode::Memory, ELRMovementPace::Walk));
	TestFalse(TEXT("Memory forbids run"), LRMovementRules::IsPaceAllowed(ELRPerceptionMode::Memory, ELRMovementPace::Run));

	TestEqual(TEXT("Normal defaults to walk"), LRMovementRules::GetDefaultPace(ELRPerceptionMode::Normal), ELRMovementPace::Walk);
	TestEqual(TEXT("Perception defaults to sneak"), LRMovementRules::GetDefaultPace(ELRPerceptionMode::Perception), ELRMovementPace::Sneak);
	TestEqual(TEXT("Courage defaults to walk"), LRMovementRules::GetDefaultPace(ELRPerceptionMode::Courage), ELRMovementPace::Walk);
	TestEqual(TEXT("Memory defaults to walk"), LRMovementRules::GetDefaultPace(ELRPerceptionMode::Memory), ELRMovementPace::Walk);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRNoiseResolverTest, "LostRunic.Movement.NoiseResolver",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRNoiseResolverTest::RunTest(const FString& parameters)
{
	ULRMovementTuning* tuning = NewObject<ULRMovementTuning>(GetTransientPackage());
	if (!TestNotNull(TEXT("Movement tuning created"), tuning))
	{
		return false;
	}

	// 潜行：任何环境都无声（半径 0 + Sneak 标签，仅供动画/表现钩子）。
	for (const ELRNoiseEnvironment environment : { ELRNoiseEnvironment::Indoor, ELRNoiseEnvironment::Outdoor,
		ELRNoiseEnvironment::OutdoorStealth })
	{
		const FLRNoiseResolution sneak = LRMovementRules::ResolveFootstepNoise(ELRMovementPace::Sneak, environment, *tuning);
		TestEqual(TEXT("Sneak radius is zero"), sneak.Radius, 0.0f);
		TestTrue(TEXT("Sneak uses sneak tag"), sneak.Tag == LRGameplayTags::NoiseFootstepSneak);
	}

	// 走路：室内 400 / 室外潜行 250 / 室外非潜行 250 + Faint。
	const FLRNoiseResolution walkIndoor = LRMovementRules::ResolveFootstepNoise(ELRMovementPace::Walk, ELRNoiseEnvironment::Indoor, *tuning);
	TestEqual(TEXT("Walk indoor radius"), walkIndoor.Radius, tuning->IndoorWalkNoiseRadius);
	TestTrue(TEXT("Walk indoor tag"), walkIndoor.Tag == LRGameplayTags::NoiseFootstepWalk);
	const FLRNoiseResolution walkStealth = LRMovementRules::ResolveFootstepNoise(ELRMovementPace::Walk, ELRNoiseEnvironment::OutdoorStealth, *tuning);
	TestEqual(TEXT("Walk outdoor stealth radius"), walkStealth.Radius, tuning->OutdoorNoiseRadius);
	TestTrue(TEXT("Walk outdoor stealth tag"), walkStealth.Tag == LRGameplayTags::NoiseFootstepWalk);
	const FLRNoiseResolution walkOpen = LRMovementRules::ResolveFootstepNoise(ELRMovementPace::Walk, ELRNoiseEnvironment::Outdoor, *tuning);
	TestEqual(TEXT("Walk outdoor open radius"), walkOpen.Radius, tuning->OutdoorNoiseRadius);
	TestTrue(TEXT("Walk outdoor open uses faint tag"), walkOpen.Tag == LRGameplayTags::NoiseFootstepWalkFaint);

	// 奔跑：室内 1200 + Run.Indoor / 室外潜行 600 / 室外非潜行 250。
	const FLRNoiseResolution runIndoor = LRMovementRules::ResolveFootstepNoise(ELRMovementPace::Run, ELRNoiseEnvironment::Indoor, *tuning);
	TestEqual(TEXT("Run indoor radius"), runIndoor.Radius, tuning->IndoorRunNoiseRadius);
	TestTrue(TEXT("Run indoor tag"), runIndoor.Tag == LRGameplayTags::NoiseFootstepRunIndoor);
	const FLRNoiseResolution runStealth = LRMovementRules::ResolveFootstepNoise(ELRMovementPace::Run, ELRNoiseEnvironment::OutdoorStealth, *tuning);
	TestEqual(TEXT("Run outdoor stealth radius"), runStealth.Radius, tuning->OutdoorStealthRunNoiseRadius);
	TestTrue(TEXT("Run outdoor stealth tag"), runStealth.Tag == LRGameplayTags::NoiseFootstepRun);
	const FLRNoiseResolution runOpen = LRMovementRules::ResolveFootstepNoise(ELRMovementPace::Run, ELRNoiseEnvironment::Outdoor, *tuning);
	TestEqual(TEXT("Run outdoor open radius"), runOpen.Radius, tuning->OutdoorNoiseRadius);
	TestTrue(TEXT("Run outdoor open tag"), runOpen.Tag == LRGameplayTags::NoiseFootstepRun);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRNoiseEnvironmentPriorityTest, "LostRunic.Movement.NoiseEnvironmentPriority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRNoiseEnvironmentPriorityTest::RunTest(const FString& parameters)
{
	const TArray<ELRNoiseEnvironment> empty;
	TestEqual(TEXT("No area defaults to outdoor"), LRMovementRules::ResolveEnvironmentFromSet(empty),
		ELRNoiseEnvironment::Outdoor);
	TestEqual(TEXT("Single indoor wins"), LRMovementRules::ResolveEnvironmentFromSet(
		{ ELRNoiseEnvironment::Indoor }), ELRNoiseEnvironment::Indoor);
	TestEqual(TEXT("Indoor beats outdoor stealth"),
		LRMovementRules::ResolveEnvironmentFromSet(
			{ ELRNoiseEnvironment::OutdoorStealth, ELRNoiseEnvironment::Indoor }),
		ELRNoiseEnvironment::Indoor);
	TestEqual(TEXT("Indoor beats outdoor"),
		LRMovementRules::ResolveEnvironmentFromSet(
			{ ELRNoiseEnvironment::Outdoor, ELRNoiseEnvironment::Indoor }),
		ELRNoiseEnvironment::Indoor);
	TestEqual(TEXT("Outdoor stealth beats outdoor"),
		LRMovementRules::ResolveEnvironmentFromSet(
			{ ELRNoiseEnvironment::Outdoor, ELRNoiseEnvironment::OutdoorStealth }),
		ELRNoiseEnvironment::OutdoorStealth);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRRoomRunAlertTargetTest, "LostRunic.Movement.RoomAlertTargets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRRoomRunAlertTargetTest::RunTest(const FString& parameters)
{
	ULRGuardTuning* tuning = NewObject<ULRGuardTuning>(GetTransientPackage());
	if (!TestNotNull(TEXT("Guard tuning created"), tuning))
	{
		return false;
	}

	// 当前房间：至少提升到 RoomRunAlertLevel(5)。
	TestEqual(TEXT("Current room raises to floor"), LRMovementRules::ResolveRoomRunTargetLevel(true, 3, *tuning), 5);
	TestEqual(TEXT("Current room at floor stays"), LRMovementRules::ResolveRoomRunTargetLevel(true, 5, *tuning), 5);
	TestEqual(TEXT("Current room above floor keeps level"), LRMovementRules::ResolveRoomRunTargetLevel(true, 7, *tuning), 7);
	// 相邻房间：max(当前, 当前+1)。
	TestEqual(TEXT("Adjacent room raises by amount"), LRMovementRules::ResolveRoomRunTargetLevel(false, 0, *tuning), 1);
	TestEqual(TEXT("Adjacent room at eight becomes nine"), LRMovementRules::ResolveRoomRunTargetLevel(false, 8, *tuning), 9);
	TestEqual(TEXT("Adjacent room at ten caps at eleven"), LRMovementRules::ResolveRoomRunTargetLevel(false, 10, *tuning), 11);
	return true;
}

#endif
