/**
 * @file LRGuardAIControllerPerception.cpp
 * @brief UE Sight/Hearing 适配、有效视觉跟踪、Grace 和噪声警戒入口。
 */
#include "AI/LRGuardAIController.h"

#include "AI/LRAlertComponent.h"
#include "AI/LRAlertRules.h"
#include "AI/LRGuardCharacter.h"
#include "AI/LRGuardKnowledgeComponent.h"
#include "AI/LRGuardPerceptionRules.h"
#include "AI/LRGuardPerceptionTarget.h"
#include "Core/LRGameplayTags.h"
#include "Core/LRTypes.h"
#include "Data/LRGuardTuning.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Perception/AIPerceptionComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Perception/AISense.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Stealth/LRGuardVisibility.h"
#include "TimerManager.h"

namespace
{
	ELRMovementPace ResolveReasonPace(const FGameplayTag reason, bool& bUsePaceMultiplier)
	{
		bUsePaceMultiplier = reason == LRGameplayTags::NoiseFootstepRun
			|| reason == LRGameplayTags::NoiseFootstepRunIndoor
			|| reason == LRGameplayTags::NoiseFootstepWalk
			|| reason == LRGameplayTags::NoiseFootstepWalkFaint
			|| reason == LRGameplayTags::NoiseFootstepSneak;
		if (reason == LRGameplayTags::NoiseFootstepRun
			|| reason == LRGameplayTags::NoiseFootstepRunIndoor)
		{
			return ELRMovementPace::Run;
		}
		if (reason == LRGameplayTags::NoiseFootstepSneak)
		{
			return ELRMovementPace::Sneak;
		}
		return ELRMovementPace::Walk;
	}
}

void ALRGuardAIController::HandlePerception(AActor* actor, const FAIStimulus stimulus)
{
	if (!IsValid(actor) || !Alert.IsValid() || !Knowledge.IsValid())
	{
		return;
	}

	if (stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
	{
		if (!IsRelevantSightTarget(actor))
		{
			return;
		}
		if (stimulus.WasSuccessfullySensed())
		{
			RawSightContact = actor;
			bHasRawSightContact = true;
			Knowledge->SetVisualCandidate(actor);
			StartSightTracking();
			HandleSightAcquiredOrTracked(actor);
		}
		else if (bHasRawSightContact && RawSightContact.Get() == actor)
		{
			const FVector location = stimulus.StimulusLocation.IsNearlyZero()
				? actor->GetActorLocation() : stimulus.StimulusLocation;
			HandleSightLost(actor, location);
		}
		return;
	}

	if (stimulus.Type != UAISense::GetSenseID<UAISense_Hearing>()
		|| !stimulus.WasSuccessfullySensed())
	{
		return;
	}

	FGameplayTag reason = FGameplayTag::RequestGameplayTag(stimulus.Tag, false);
	if (!reason.IsValid())
	{
		reason = LRGameplayTags::NoiseInteraction;
	}

	FLRGuardNoiseStimulus noise;
	noise.Source = actor;
	noise.Location = stimulus.StimulusLocation;
	noise.Reason = reason;
	noise.PropagationMode = ELRGuardNoisePropagationMode::Hearing;
	// Hearing transports the immutable footstep Reason; convert it to the event snapshot
	// before the controller applies any alert rule. Non-footstep reasons stay neutral.
	noise.SourcePace = ResolveReasonPace(reason, noise.bHasSourcePace);
	noise.TimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	ReceiveNoiseStimulus(noise);
}

bool ALRGuardAIController::CanCurrentlySeeTarget() const
{
	AActor* target = RawSightContact.Get();
	return bHasRawSightContact && IsRelevantSightTarget(target) && !IsHiddenFromGuard(target);
}

void ALRGuardAIController::HandleSightAcquiredOrTracked(AActor* actor)
{
	if (!IsValid(actor) || !Alert.IsValid() || !Knowledge.IsValid())
	{
		return;
	}

	if (!CanCurrentlySeeTarget())
	{
		if (Knowledge->IsCurrentlyVisible())
		{
			Knowledge->RecordSightLost(Knowledge->GetLastKnownThreatLocation());
			ProcessAwarenessTransaction(LRGameplayTags::SightPlayerLost);
		}
		return;
	}

	const FVector location = actor->GetActorLocation();
	Knowledge->RecordVisibleThreat(actor, location);
	const int32 alertLevel = Alert->GetAlertLevel();
	if (alertLevel <= LRAlertRules::SuspiciousMaxLevel)
	{
		// Any valid sight from the white band starts one fresh grace window.
		// The consumed flag is reset on white-band entry, including red decay 6 -> 5.
		bSightToChaseGraceConsumed = false;
		Alert->ApplySightAlertLevel();
		StartSightToChaseGrace();
	}
	else if (alertLevel <= LRAlertRules::InvestigateMaxLevel)
	{
		if (!bSightToChaseGraceActive)
		{
			Knowledge->SetConfirmedThreat(actor, location);
			Alert->ApplyDelta(LRAlertRules::ConfirmedAlertLevel - alertLevel);
			Alert->StopObservationAndDecay();
		}
	}
	else
	{
		Knowledge->SetConfirmedThreat(actor, location);
	}

	ProcessAwarenessTransaction(LRGameplayTags::SightPlayer);
	HandleCaptureCheck();
}

void ALRGuardAIController::HandleSightLost(AActor* actor, const FVector& lastKnownLocation)
{
	if (!Alert.IsValid() || !Knowledge.IsValid()
		|| (actor && bHasRawSightContact && RawSightContact.Get() != actor))
	{
		return;
	}

	HandleCaptureCheck();
	const FVector location = lastKnownLocation.IsNearlyZero() && IsValid(actor)
		? actor->GetActorLocation() : lastKnownLocation;
	Knowledge->RecordSightLost(location);
	Knowledge->SetVisualCandidate(nullptr);
	RawSightContact.Reset();
	bHasRawSightContact = false;
	StopSightTracking();

	if (Alert->GetAlertLevel() == LRAlertRules::ConfirmedAlertLevel)
	{
		Alert->ApplyDelta(-1);
		ClearInvestigationMoveRequest();
	}
	ProcessAwarenessTransaction(LRGameplayTags::SightPlayerLost, true);
}

void ALRGuardAIController::HandleSightTracking()
{
	if (!bHasRawSightContact || !Alert.IsValid() || !Knowledge.IsValid())
	{
		return;
	}

	AActor* target = RawSightContact.Get();
	if (!IsRelevantSightTarget(target))
	{
		HandleSightLost(target, Knowledge->GetLastKnownThreatLocation());
		return;
	}

	if (CanCurrentlySeeTarget())
	{
		HandleSightAcquiredOrTracked(target);
		return;
	}

	const bool bWasVisible = Knowledge->IsCurrentlyVisible();
	if (bWasVisible)
	{
		Knowledge->RecordSightLost(target->GetActorLocation());
	}
	if (Alert->GetAlertLevel() == LRAlertRules::ConfirmedAlertLevel)
	{
		Alert->ApplyDelta(-1);
		ClearInvestigationMoveRequest();
	}
	if (bWasVisible || Alert->GetAlertLevel() == LRAlertRules::InvestigateMaxLevel)
	{
		ProcessAwarenessTransaction(LRGameplayTags::SightPlayerLost, true);
	}
}

void ALRGuardAIController::StartSightTracking()
{
	if (!GetWorld() || !bHasRawSightContact)
	{
		return;
	}

	FTimerManager& timers = GetWorld()->GetTimerManager();
	if (!timers.IsTimerActive(SightTrackingTimer))
	{
		timers.SetTimer(SightTrackingTimer, this, &ALRGuardAIController::HandleSightTracking,
			GetEffectiveTuning().SightTrackingIntervalSeconds, true);
	}
}

void ALRGuardAIController::StopSightTracking()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(SightTrackingTimer);
	}
}

void ALRGuardAIController::StartSightToChaseGrace()
{
	if (!GetWorld() || bSightToChaseGraceActive || bSightToChaseGraceConsumed)
	{
		return;
	}

	bSightToChaseGraceConsumed = true;
	bSightToChaseGraceActive = true;
	GetWorld()->GetTimerManager().ClearTimer(SightToChaseGraceTimer);
	const float duration = GetEffectiveTuning().SightToChaseGraceSeconds;
	if (duration <= 0.0f)
	{
		HandleSightGraceExpired();
		return;
	}
	GetWorld()->GetTimerManager().SetTimer(SightToChaseGraceTimer, this,
		&ALRGuardAIController::HandleSightGraceExpired, duration, false);
}

void ALRGuardAIController::StopSightToChaseGrace()
{
	bSightToChaseGraceActive = false;
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(SightToChaseGraceTimer);
	}
}

void ALRGuardAIController::HandleSightGraceExpired()
{
	if (!bSightToChaseGraceActive)
	{
		return;
	}

	bSightToChaseGraceActive = false;
	if (!Alert.IsValid() || !Knowledge.IsValid()
		|| Alert->GetAlertLevel() < LRAlertRules::InvestigateMinLevel
		|| Alert->GetAlertLevel() > LRAlertRules::InvestigateMaxLevel)
	{
		return;
	}

	if (CanCurrentlySeeTarget())
	{
		AActor* target = RawSightContact.Get();
		Knowledge->RecordVisibleThreat(target, target->GetActorLocation());
		Knowledge->SetConfirmedThreat(target, target->GetActorLocation());
		Alert->ApplyDelta(LRAlertRules::ConfirmedAlertLevel - Alert->GetAlertLevel());
		Alert->StopObservationAndDecay();
		ProcessAwarenessTransaction(LRGameplayTags::SightPlayer, true);
		HandleCaptureCheck();
		return;
	}

	if (InvestigationMoveStatus == ELRGuardInvestigationMoveStatus::AtLocation
		|| InvestigationMoveStatus == ELRGuardInvestigationMoveStatus::Failed)
	{
		// Grace owns the freeze; after it ends, an unreachable point observes in place
		// without pretending the navigation request reached its destination.
		RequestInvestigationObservationIfReady();
	}
	else if (InvestigationMoveStatus == ELRGuardInvestigationMoveStatus::None
		&& Knowledge->GetSnapshot().bHasLatestInvestigationLocation)
	{
		RequestInvestigationMove(Knowledge->GetLatestInvestigationLocation());
	}
	ProcessAwarenessTransaction(LRGameplayTags::SightPlayerLost, true);
}

void ALRGuardAIController::HandleAttractStimulus(const FLRGuardNoiseStimulus& stimulus)
{
	if (!Alert.IsValid() || !Knowledge.IsValid() || !stimulus.Reason.IsValid()
		|| Alert->GetAlertLevel() >= LRAlertRules::ConfirmedAlertLevel)
	{
		return;
	}

	if (bSightToChaseGraceActive)
	{
		Knowledge->RecordDisturbance(stimulus, false);
		ProcessAwarenessTransaction(stimulus.Reason);
		return;
	}

	const int32 currentAlert = Alert->GetAlertLevel();
	const FLRNoiseResponse response = LRGuardPerceptionRules::ResolveNoiseAlertDelta(
		stimulus.Reason, stimulus.PropagationMode, currentAlert, GetEffectiveTuning());
	if (!response.bRespond)
	{
		return;
	}

	const double nowSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : stimulus.TimeSeconds;
	if (!Alert->CanAcceptAttract(nowSeconds))
	{
		return;
	}

	const int32 resultAlert = LRGuardPerceptionRules::ResolveNoiseResultLevel(
		currentAlert, response, GetEffectiveTuning());
	const bool bFirstAttractInBand = Alert->IsFirstAttractInResultBand(resultAlert);
	bool bUsePaceMultiplier = stimulus.bHasSourcePace;
	const ELRMovementPace sourcePace = stimulus.bHasSourcePace
		? stimulus.SourcePace : ResolveReasonPace(stimulus.Reason, bUsePaceMultiplier);
	const float cooldown = LRAlertRules::ResolveAttractCooldown(
		resultAlert, bFirstAttractInBand, sourcePace, bUsePaceMultiplier, GetEffectiveTuning());
	Alert->ApplyAcceptedAttract(resultAlert, nowSeconds, cooldown,
		resultAlert <= LRAlertRules::SuspiciousMaxLevel);
	Knowledge->RecordDisturbance(stimulus, !Knowledge->IsCurrentlyVisible());
	if (resultAlert >= LRAlertRules::InvestigateMinLevel)
	{
		bForceInvestigationRetarget = true;
	}
	ProcessAwarenessTransaction(stimulus.Reason);
}

void ALRGuardAIController::HandleCaptureCheck()
{
	if (!Alert.IsValid() || !Knowledge.IsValid() || bStunned)
	{
		return;
	}

	const FLRAlertSnapshot alertSnapshot = Alert->GetAlertSnapshot();
	const FLRGuardKnowledgeSnapshot knowledgeSnapshot = Knowledge->GetSnapshot();
	if (!LRAlertRules::IsChaseEligible(alertSnapshot, knowledgeSnapshot))
	{
		return;
	}

	AActor* target = knowledgeSnapshot.ConfirmedThreat.Get();
	ALRGuardCharacter* guard = Cast<ALRGuardCharacter>(GetPawn());
	if (guard && IsValid(target)
		&& FVector::Dist2D(guard->GetActorLocation(), target->GetActorLocation())
			<= GetEffectiveTuning().CaptureRadius)
	{
		guard->CaptureTarget(target);
	}
}

bool ALRGuardAIController::IsHiddenFromGuard(AActor* actor) const
{
	if (!actor)
	{
		return true;
	}
	if (actor->GetClass()->ImplementsInterface(ULRGuardVisibility::StaticClass()))
	{
		return !ILRGuardVisibility::Execute_IsVisibleToGuard(actor,
			const_cast<ALRGuardAIController*>(this));
	}
	for (UActorComponent* component : actor->GetComponents())
	{
		if (component && component->GetClass()->ImplementsInterface(ULRGuardVisibility::StaticClass())
			&& !ILRGuardVisibility::Execute_IsVisibleToGuard(component,
				const_cast<ALRGuardAIController*>(this)))
		{
			return true;
		}
	}
	return false;
}
