/**
 * @file LRGuardAwarenessTests.cpp
 * @brief Pure continuous-sight and snapshot-priority automation tests.
 */
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AI/LRAlertRules.h"
#include "AI/LRGuardPerceptionRules.h"
#include "Data/LRGuardTuning.h"
#include "GameFramework/Actor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRGuardContinuousSightRulesTest, "LostRunic.AI.ContinuousSightRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRGuardContinuousSightRulesTest::RunTest(const FString& parameters)
{
	ULRGuardTuning* tuning = NewObject<ULRGuardTuning>(GetTransientPackage());
	if (!TestNotNull(TEXT("Guard tuning created"), tuning))
	{
		return false;
	}

	const float boundaryDot = FMath::Cos(FMath::DegreesToRadians(tuning->SightConeDegrees * 0.5f));
	const FLRGuardVisibilityResult edgeSample = LRGuardPerceptionRules::EvaluateVisibility(
		tuning->SightRadius, boundaryDot, true, true, true, 1.0f, 1.0f, 1.0f, 1.0f, *tuning);
	TestTrue(TEXT("All sight gates pass at the radius boundary"), edgeSample.IsActive());
	TestEqual(TEXT("Distance factor uses the configured edge multiplier"), edgeSample.DistanceFactor,
		tuning->SightEdgeDetectionMultiplier, 0.001f);
	TestEqual(TEXT("Edge score includes all factors"), edgeSample.VisibilityScore,
		tuning->SightEdgeDetectionMultiplier, 0.001f);

	const FLRGuardVisibilityResult beyondRange = LRGuardPerceptionRules::EvaluateVisibility(
		tuning->SightRadius + 1.0f, 1.0f, true, true, true, 1.0f, 1.0f, 1.0f, 1.0f, *tuning);
	TestFalse(TEXT("Distance beyond SightRadius fails the range gate"), beyondRange.bRangeGate);
	TestEqual(TEXT("Distance beyond SightRadius has zero score"), beyondRange.VisibilityScore, 0.0f, 0.001f);

	const FLRGuardVisibilityResult blocked = LRGuardPerceptionRules::EvaluateVisibility(
		100.0f, 1.0f, true, false, true, 1.0f, 1.0f, 1.0f, 1.0f, *tuning);
	TestFalse(TEXT("LOS gate blocks score"), blocked.bLOSGate);
	TestEqual(TEXT("Any failed gate produces zero score"), blocked.VisibilityScore, 0.0f, 0.001f);

	const FLRGuardVisibilityResult weighted = LRGuardPerceptionRules::EvaluateVisibility(
		0.0f, 1.0f, true, true, true, 0.5f, 0.8f, 0.75f, 1.0f, *tuning);
	TestEqual(TEXT("Factors multiply after gates"), weighted.VisibilityScore, 0.3f, 0.001f);

	TestEqual(TEXT("Suspicious threshold resolves once"),
		LRGuardPerceptionRules::ResolveDetectionStage(tuning->SuspiciousExposureThresholdSeconds, *tuning),
		ELRGuardDetectionStage::Suspicious);
	TestEqual(TEXT("Investigate threshold resolves consistently"),
		LRGuardPerceptionRules::ResolveDetectionStage(tuning->InvestigateExposureThresholdSeconds, *tuning),
		ELRGuardDetectionStage::Investigate);
	TestEqual(TEXT("Repeated stage resolution has no transition side effect"),
		LRGuardPerceptionRules::ResolveDetectionStage(tuning->InvestigateExposureThresholdSeconds, *tuning),
		LRGuardPerceptionRules::ResolveDetectionStage(tuning->InvestigateExposureThresholdSeconds, *tuning));
	TestEqual(TEXT("Confirmed threshold resolves"),
		LRGuardPerceptionRules::ResolveDetectionStage(tuning->ConfirmedExposureThresholdSeconds, *tuning),
		ELRGuardDetectionStage::Confirmed);
	TestEqual(TEXT("Visibility integrates score-weighted exposure"),
		LRGuardPerceptionRules::IntegrateDetectionExposure(0.0f, weighted, 0.2f, *tuning), 0.06f, 0.001f);
	TestEqual(TEXT("Inactive visibility decays exposure"),
		LRGuardPerceptionRules::IntegrateDetectionExposure(0.5f, blocked, 0.2f, *tuning), 0.3f, 0.001f);
	const FLRGuardVisibilityResult fullyVisible = LRGuardPerceptionRules::EvaluateVisibility(
		0.0f, 1.0f, true, true, true, 1.0f, 1.0f, 1.0f, 1.0f, *tuning);
	TestEqual(TEXT("Integration clamps a long sample to MaxDetectionIntegrationDeltaSeconds"),
		LRGuardPerceptionRules::IntegrateDetectionExposure(0.0f, fullyVisible, 1.0f, *tuning),
		0.2f, 0.001f);
	TestEqual(TEXT("Inactive integration clamps exposure at zero"),
		LRGuardPerceptionRules::IntegrateDetectionExposure(0.1f, blocked, 1.0f, *tuning),
		0.0f, 0.001f);
	TestEqual(TEXT("Stage resolves down to Suspicious after exposure falls"),
		LRGuardPerceptionRules::ResolveDetectionStage(0.4f, *tuning), ELRGuardDetectionStage::Suspicious);
	TestEqual(TEXT("Stage resolves to None at zero exposure"),
		LRGuardPerceptionRules::ResolveDetectionStage(0.0f, *tuning), ELRGuardDetectionStage::None);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRGuardAwarenessSnapshotRulesTest, "LostRunic.AI.AwarenessSnapshotRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRGuardAwarenessSnapshotRulesTest::RunTest(const FString& parameters)
{
	ULRGuardTuning* tuning = NewObject<ULRGuardTuning>(GetTransientPackage());
	if (!TestNotNull(TEXT("Guard tuning created"), tuning))
	{
		return false;
	}

	FLRAlertSnapshot alert;
	FLRGuardKnowledgeSnapshot knowledge;
	knowledge.bHasConfirmedThreat = true;
	AActor* confirmedThreat = NewObject<AActor>(GetTransientPackage());
	AActor* otherActor = NewObject<AActor>(GetTransientPackage());
	knowledge.ConfirmedThreat = confirmedThreat;
	knowledge.VisualCandidate = confirmedThreat;
	knowledge.bHasVisualCandidate = true;
	knowledge.CurrentVisibility = LRGuardPerceptionRules::EvaluateVisibility(
		0.0f, 1.0f, true, true, true, 1.0f, 1.0f, 1.0f, 1.0f, *tuning);
	alert.Level = 11;
	TestEqual(TEXT("Stunned has highest priority"),
		LRAlertRules::ResolveTargetBehavior(true, alert, knowledge, false, *tuning),
		ELRGuardBehaviorState::Stunned);
	TestEqual(TEXT("Visible confirmed threat at chase floor chases"),
		LRAlertRules::ResolveTargetBehavior(false, alert, knowledge, false, *tuning),
		ELRGuardBehaviorState::Chase);

	knowledge.VisualCandidate = otherActor;
	TestEqual(TEXT("Historical threat memory does not chase a different visible candidate"),
		LRAlertRules::ResolveTargetBehavior(false, alert, knowledge, false, *tuning),
		ELRGuardBehaviorState::Search);
	knowledge.CurrentVisibility = FLRGuardVisibilityResult();
	TestEqual(TEXT("Alert 11 alone never chases"),
		LRAlertRules::ResolveTargetBehavior(false, alert, knowledge, false, *tuning),
		ELRGuardBehaviorState::Search);

	alert.Level = 0;
	knowledge.bPendingThreatInvestigation = true;
	TestEqual(TEXT("Zero alert resolves idle before investigation"),
		LRAlertRules::ResolveTargetBehavior(false, alert, knowledge, false, *tuning),
		ELRGuardBehaviorState::IdlePatrol);

	alert.Level = 6;
	TestEqual(TEXT("Pending threat at investigate floor investigates"),
		LRAlertRules::ResolveTargetBehavior(false, alert, knowledge, false, *tuning),
		ELRGuardBehaviorState::Investigate);
	knowledge.bPendingThreatInvestigation = false;
	TestEqual(TEXT("Search flag in red band searches"),
		LRAlertRules::ResolveTargetBehavior(false, alert, knowledge, true, *tuning),
		ELRGuardBehaviorState::Search);
	tuning->DetectionInvestigateAlertFloor = 7;
	TestEqual(TEXT("Search flag uses the configured red-band lower bound"),
		LRAlertRules::ResolveTargetBehavior(false, alert, knowledge, true, *tuning),
		ELRGuardBehaviorState::Suspicious);
	alert.Level = 5;
	TestEqual(TEXT("Search flag does not include white band"),
		LRAlertRules::ResolveTargetBehavior(false, alert, knowledge, true, *tuning),
		ELRGuardBehaviorState::Suspicious);
	tuning->DetectionInvestigateAlertFloor = 6;
	TestEqual(TEXT("One to five resolves suspicious"),
		LRAlertRules::ResolveTargetBehavior(false, alert, knowledge, false, *tuning),
		ELRGuardBehaviorState::Suspicious);
	alert.Level = 6;
	TestEqual(TEXT("Six to ten resolves investigate without a pending flag"),
		LRAlertRules::ResolveTargetBehavior(false, alert, knowledge, false, *tuning),
		ELRGuardBehaviorState::Investigate);

	alert.Level = 5;
	knowledge.CurrentVisibility = LRGuardPerceptionRules::EvaluateVisibility(
		0.0f, 1.0f, true, true, true, 1.0f, 1.0f, 1.0f, 1.0f, *tuning);
	TestFalse(TEXT("Active current visibility blocks decay"),
		LRAlertRules::ShouldDecay(alert, knowledge, false, ELRGuardBehaviorState::Suspicious));
	knowledge.CurrentVisibility = FLRGuardVisibilityResult();
	TestTrue(TEXT("Suspicious alert decays without active visibility"),
		LRAlertRules::ShouldDecay(alert, knowledge, false, ELRGuardBehaviorState::Suspicious));
	TestFalse(TEXT("Observation blocks snapshot decay"),
		LRAlertRules::ShouldDecay(alert, knowledge, true, ELRGuardBehaviorState::Suspicious));

	knowledge.bPendingThreatInvestigation = true;
	knowledge.bHasLastKnownThreatLocation = true;
	knowledge.LastKnownThreatLocation = FVector(10.0f, 0.0f, 0.0f);
	knowledge.bHasLastDisturbanceLocation = true;
	knowledge.LastDisturbanceLocation = FVector(20.0f, 0.0f, 0.0f);
	TestEqual(TEXT("Pending threat location has priority"), LRAlertRules::ResolveInvestigationLocation(knowledge),
		FVector(10.0f, 0.0f, 0.0f));
	knowledge.bPendingThreatInvestigation = false;
	TestEqual(TEXT("Disturbance is used when no threat is pending"), LRAlertRules::ResolveInvestigationLocation(knowledge),
		FVector(20.0f, 0.0f, 0.0f));
	knowledge.bHasLastDisturbanceLocation = false;
	TestEqual(TEXT("Last known location is the final fallback"), LRAlertRules::ResolveInvestigationLocation(knowledge),
		FVector(10.0f, 0.0f, 0.0f));

	FLRGuardKnowledgeSnapshot previousPending;
	previousPending.InvestigationContextRevision = 4;
	FLRGuardKnowledgeSnapshot changedPending = previousPending;
	changedPending.InvestigationContextRevision = 5;
	TestTrue(TEXT("Accepted navigation revision advances investigation context"),
		LRGuardPerceptionRules::HasNewInvestigationContext(previousPending, changedPending));
	TestFalse(TEXT("Same navigation revision does not advance context"),
		LRGuardPerceptionRules::HasNewInvestigationContext(previousPending, previousPending));
	return true;
}

#endif
