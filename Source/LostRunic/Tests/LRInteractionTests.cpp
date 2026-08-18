/**
 * @file LRInteractionTests.cpp
 * @brief 提供 LostRunic Runtime 自动化测试，覆盖调优边界、状态矩阵、交互筛选、物品双入口、守卫警戒、叙事分支和存档事务顺序。仅在 WITH_DEV_AUTOMATION_TESTS 下编译。
 *
 * 关联文件：Tests 目录内调用该公共契约的实现文件；所属领域：Tests。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Data/LRInteractionTuning.h"
#include "Components/SceneComponent.h"
#include "Interaction/LRInteractionPresentationComponent.h"
#include "Interaction/LRInteractionRules.h"
#include "Interaction/LRInteractionTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRInteractionCandidateTest, "LostRunic.Interaction.SelectsNearestLegalCandidate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRInteractionCandidateTest::RunTest(const FString& parameters)
{
	const ULRInteractionTuning* tuning = GetDefault<ULRInteractionTuning>();
	TArray<FLRInteractionCandidateScore> candidates;
	candidates.Add({ FMath::Square(180.0f), 1.0f, true, true, true });
	candidates.Add({ FMath::Square(350.0f), 1.0f, false, true, true });
	candidates.Add({ FMath::Square(120.0f), 0.0f, false, true, true });
	candidates.Add({ FMath::Square(90.0f), 1.0f, false, false, true });
	candidates.Add({ FMath::Square(70.0f), 1.0f, false, true, false });

	TestEqual(TEXT("Nearest candidate surviving occlusion, facing, state, and item rules is selected"),
		LRInteractionRules::SelectBestCandidate(candidates, *tuning), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRInteractionRangeTest, "LostRunic.Interaction.DistanceBandsAndFacingBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRInteractionRangeTest::RunTest(const FString& parameters)
{
	const ULRInteractionTuning* tuning = GetDefault<ULRInteractionTuning>();
	TestEqual(TEXT("Beyond the outline radius uses the far hint"),
		LRInteractionRules::GetPresentationState(FMath::Square(tuning->OutlineDistance + 0.1f), *tuning),
		ELRInteractionPresentationState::FarHint);
	TestEqual(TEXT("Outline distance uses the outline state"),
		LRInteractionRules::GetPresentationState(FMath::Square(tuning->OutlineDistance), *tuning),
		ELRInteractionPresentationState::NearOutline);
	TestEqual(TEXT("Far hint boundary stays visible"),
		LRInteractionRules::GetPresentationState(FMath::Square(tuning->FarHintDistance), *tuning),
		ELRInteractionPresentationState::FarHint);
	TestEqual(TEXT("Beyond the far hint is hidden"),
		LRInteractionRules::GetPresentationState(FMath::Square(tuning->FarHintDistance + 0.1f), *tuning),
		ELRInteractionPresentationState::None);
	TestTrue(TEXT("Execution boundary is included"),
		LRInteractionRules::IsWithinExecutionDistance(FMath::Square(tuning->ExecuteDistance), tuning->ExecuteDistance));
	TestFalse(TEXT("Outside execution boundary is rejected"),
		LRInteractionRules::IsWithinExecutionDistance(FMath::Square(tuning->ExecuteDistance + 0.1f), tuning->ExecuteDistance));

	const float boundaryDot = FMath::Cos(FMath::DegreesToRadians(tuning->FacingConeDegrees * 0.5f));
	const float insideDot = FMath::Cos(FMath::DegreesToRadians(44.9f));
	const float outsideDot = FMath::Cos(FMath::DegreesToRadians(45.1f));
	TestTrue(TEXT("44.9 degree target is inside the half-angle"),
		LRInteractionRules::IsFacingAllowed(insideDot, *tuning));
	TestTrue(TEXT("45.0 degree boundary is included"),
		LRInteractionRules::IsFacingAllowed(boundaryDot, *tuning));
	TestFalse(TEXT("45.1 degree target is outside the half-angle"),
		LRInteractionRules::IsFacingAllowed(outsideDot, *tuning));
	TestTrue(TEXT("The same 44.9 degree rule applies on the opposite side"),
		LRInteractionRules::IsFacingAllowed(insideDot, *tuning));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRInteractionPromptPresentationConfigTest,
	"LostRunic.Interaction.PromptPresentationConfig", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRInteractionPromptPresentationConfigTest::RunTest(const FString& parameters)
{
	const ULRInteractionTuning* tuning = GetDefault<ULRInteractionTuning>();
	TestEqual(TEXT("Default prompt Z offset is 40 cm"), tuning->InteractionPromptZOffset, 40.0f);

	AActor* actor = NewObject<AActor>();
	USceneComponent* root = NewObject<USceneComponent>(actor, TEXT("PromptRoot"));
	actor->SetRootComponent(root);
	actor->AddInstanceComponent(root);
	ULRInteractionPresentationComponent* presentation = NewObject<ULRInteractionPresentationComponent>(actor);
	TestEqual(TEXT("Missing Presentation anchor override falls back to caller anchor"),
		presentation->ResolvePromptAnchorComponent(root), root);

	presentation->PromptAnchorOverride.PathToComponent = TEXT("PromptRoot");
	TestEqual(TEXT("FComponentReference resolves the named SceneComponent"),
		presentation->ResolvePromptAnchorComponent(nullptr), root);
	presentation->bOverridePromptZOffset = true;
	presentation->PromptZOffsetOverride = 75.0f;
	TestEqual(TEXT("Instance prompt Z offset overrides the shared tuning value"),
		presentation->ResolvePromptZOffset(tuning->InteractionPromptZOffset), 75.0f);

	FLRInteractionPromptView before;
	before.Target = actor;
	before.Prompt = FText::FromString(TEXT("打开"));
	before.bVisible = true;
	FLRInteractionPromptView after = before;
	after.Prompt = FText::FromString(TEXT("关闭"));
	TestFalse(TEXT("Same target with changed prompt text is not treated as the same presentation"),
		before.HasSamePresentationAs(after));
	return true;
}

#endif
