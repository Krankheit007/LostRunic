/**
 * @file LRGuardAIControllerPerception.cpp
 * @brief UE perception adapter, target filtering and timer-driven continuous detection.
 */
#include "AI/LRGuardAIController.h"

#include "AI/LRAlertComponent.h"
#include "AI/LRGuardCharacter.h"
#include "AI/LRGuardKnowledgeComponent.h"
#include "AI/LRGuardPerceptionRules.h"
#include "AI/LRGuardPerceptionTarget.h"
#include "Core/LRGameplayTags.h"
#include "Core/LRTypes.h"
#include "Data/LRGuardTuning.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Gameplay/LRLocomotionComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Stealth/LRGuardVisibility.h"

void ALRGuardAIController::HandlePerception(AActor* actor, const FAIStimulus stimulus)
{
	if (!actor || !Alert.IsValid() || !Knowledge.IsValid())
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
			PerceivedSightContact = actor;
			Knowledge->SetVisualCandidate(actor);
			LastDetectionSampleTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
			StartDetectionSampling();
			ProcessAwarenessTransaction(LRGameplayTags::SightPlayer);
		}
		else if (PerceivedSightContact.Get() == actor)
		{
			const FVector location = stimulus.StimulusLocation.IsNearlyZero()
				? actor->GetActorLocation() : stimulus.StimulusLocation;
			HandleSightLost(actor, location);
		}
		return;
	}
	if (stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>() && stimulus.WasSuccessfullySensed())
	{
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
		noise.TimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
		ReceiveNoiseStimulus(noise);
	}
}
bool ALRGuardAIController::IsRelevantSightTarget(const AActor* actor) const
{
	if (!IsValid(actor)
		|| !actor->GetClass()->ImplementsInterface(ULRGuardPerceptionTarget::StaticClass()))
	{
		return false;
	}

	static const FName targetFunctionName =
		GET_FUNCTION_NAME_CHECKED(ILRGuardPerceptionTarget, IsRelevantGuardSightTarget);
	if (actor->GetClass()->IsFunctionImplementedInScript(targetFunctionName))
	{
		return ILRGuardPerceptionTarget::Execute_IsRelevantGuardSightTarget(actor);
	}

	const ILRGuardPerceptionTarget* target = Cast<ILRGuardPerceptionTarget>(actor);
	return target && target->IsRelevantGuardSightTarget_Implementation();
}

void ALRGuardAIController::HandleDetectionSample()
{
	if (!Alert.IsValid() || !Knowledge.IsValid() || !GetWorld())
	{
		return;
	}
	const double now = GetWorld()->GetTimeSeconds();
	const float actualDelta = LastDetectionSampleTime > 0.0
		? FMath::Max(static_cast<float>(now - LastDetectionSampleTime), 0.0f)
		: GetEffectiveTuning().DetectionSampleIntervalSeconds;
	LastDetectionSampleTime = now;
	const FLRGuardAwarenessSnapshot previous = BuildCurrentAwarenessSnapshot();
	AActor* candidate = PerceivedSightContact.Get();
	if (!IsRelevantSightTarget(candidate))
	{
		if (Knowledge->HasVisualCandidate() || candidate)
		{
			const FVector location = IsValid(candidate) ? candidate->GetActorLocation()
				: Knowledge->GetLastKnownThreatLocation();
			Knowledge->RecordSightLoss(location);
			PerceivedSightContact.Reset();
		}
		Knowledge->ApplyVisibilitySample(FLRGuardVisibilityResult(), actualDelta, GetEffectiveTuning());
		ProcessAwarenessTransaction(LRGameplayTags::SightPlayerLost);
		HandleCaptureCheck();
		if (!Knowledge->HasVisualCandidate()
			&& Knowledge->GetEffectiveExposureSeconds() <= KINDA_SMALL_NUMBER)
		{
			StopDetectionSampling();
		}
		return;
	}

	const ELRGuardDetectionStage previousStage = previous.Knowledge.Stage;
	const FLRGuardVisibilityResult visibility = EvaluateVisibility(candidate);
	Knowledge->ApplyVisibilitySample(visibility, actualDelta, GetEffectiveTuning());
	const ELRGuardDetectionStage currentStage = Knowledge->GetDetectionStage();
	if (!visibility.IsActive())
	{
		// A zero project visibility sample is not a UE Sight Lost event. Keep the raw
		// perception contact so a later sample can reacquire without another UE event.
		ProcessAwarenessTransaction(LRGameplayTags::SightPlayer);
		HandleCaptureCheck();
		return;
	}

	const bool bWasConfirmedThreat = Knowledge->HasConfirmedThreatActor(candidate);
	if (currentStage == ELRGuardDetectionStage::Confirmed)
	{
		Knowledge->SetConfirmedThreat(candidate, candidate->GetActorLocation());
	}
	else if (static_cast<uint8>(currentStage) >= static_cast<uint8>(ELRGuardDetectionStage::Investigate))
	{
		Knowledge->RecordVisualEvidence(candidate, candidate->GetActorLocation(),
			ShouldRetryInvestigationAt(candidate->GetActorLocation()));
	}
	else if (currentStage == ELRGuardDetectionStage::Suspicious)
	{
		Knowledge->RecordVisualEvidence(candidate, candidate->GetActorLocation(), false);
	}
	if (bWasConfirmedThreat)
	{
		Alert->RaiseToMinimum(GetEffectiveTuning().DetectionConfirmedAlertFloor);
	}
	if (static_cast<uint8>(currentStage) > static_cast<uint8>(previousStage))
	{
		int32 floor = 0;
		switch (currentStage)
		{
		case ELRGuardDetectionStage::Suspicious:
			floor = GetEffectiveTuning().DetectionSuspiciousAlertFloor;
			break;
		case ELRGuardDetectionStage::Investigate:
			floor = GetEffectiveTuning().DetectionInvestigateAlertFloor;
			break;
		case ELRGuardDetectionStage::Confirmed:
			floor = GetEffectiveTuning().DetectionConfirmedAlertFloor;
			break;
		default:
			break;
		}
		Alert->RaiseToMinimum(floor);
	}
	ProcessAwarenessTransaction(LRGameplayTags::SightPlayer);
	HandleCaptureCheck();
}
void ALRGuardAIController::HandleSightLost(AActor* actor, const FVector& lastKnownLocation)
{
	if (!Alert.IsValid() || !Knowledge.IsValid())
	{
		return;
	}
	// UE Sight can drop a close target below a narrow vertical cone before the
	// path-following completion callback runs. Resolve an already-earned capture
	// against the last active visibility sample before committing sight loss.
	HandleCaptureCheck();
	Knowledge->RecordSightLoss(lastKnownLocation);
	if (!actor || PerceivedSightContact.Get() == actor)
	{
		PerceivedSightContact.Reset();
	}
	ProcessAwarenessTransaction(LRGameplayTags::SightPlayerLost);
	if (Knowledge->GetEffectiveExposureSeconds() <= KINDA_SMALL_NUMBER
		&& !Knowledge->HasVisualCandidate())
	{
		StopDetectionSampling();
	}
}
FLRGuardVisibilityResult ALRGuardAIController::EvaluateVisibility(AActor* actor) const
{
	const APawn* guardPawn = GetPawn();
	const UAISenseConfig_Sight* sight = AIPerception
		? AIPerception->GetSenseConfig<UAISenseConfig_Sight>() : nullptr;
	if (!actor || !guardPawn || !sight)
	{
		return FLRGuardVisibilityResult();
	}
	const FVector toTarget = actor->GetActorLocation() - guardPawn->GetActorLocation();
	const float distance = toTarget.Size2D();
	const float forwardDot = FVector::DotProduct(guardPawn->GetActorForwardVector().GetSafeNormal2D(),
		toTarget.GetSafeNormal2D());
	return LRGuardPerceptionRules::EvaluateVisibility(distance, forwardDot,
		sight->SightRadius, sight->PeripheralVisionAngleDegrees,
		PerceivedSightContact.Get() == actor, LineOfSightTo(actor), !IsHiddenFromGuard(actor),
		ResolveMovementVisibilityFactor(actor), 1.0f, 1.0f, 1.0f, GetEffectiveTuning());
}

float ALRGuardAIController::ResolveMovementVisibilityFactor(const AActor* actor) const
{
	const ULRLocomotionComponent* locomotion = actor
		? actor->FindComponentByClass<ULRLocomotionComponent>() : nullptr;
	if (!locomotion)
	{
		return GetEffectiveTuning().WalkVisibilityMultiplier;
	}
	switch (locomotion->GetPace())
	{
	case ELRMovementPace::Sneak:
		return GetEffectiveTuning().SneakVisibilityMultiplier;
	case ELRMovementPace::Run:
		return GetEffectiveTuning().RunVisibilityMultiplier;
	default:
		return GetEffectiveTuning().WalkVisibilityMultiplier;
	}
}

void ALRGuardAIController::StartDetectionSampling()
{
	if (!GetWorld())
	{
		return;
	}
	FTimerManager& timers = GetWorld()->GetTimerManager();
	if (!timers.IsTimerActive(DetectionSampleTimer))
	{
		timers.SetTimer(DetectionSampleTimer, this, &ALRGuardAIController::HandleDetectionSample,
			Tuning.DetectionSampleIntervalSeconds, true);
	}
}

void ALRGuardAIController::StopDetectionSampling()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(DetectionSampleTimer);
	}
}
void ALRGuardAIController::HandleCaptureCheck()
{
	const FLRGuardAwarenessSnapshot awareness = BuildCurrentAwarenessSnapshot();
	AActor* target = awareness.Knowledge.ConfirmedThreat.Get();
	if (bStunned || awareness.ResolvedBehavior != ELRGuardBehaviorState::Chase
		|| !awareness.Knowledge.CurrentVisibility.IsActive() || !IsValid(target))
	{
		return;
	}
	ALRGuardCharacter* guard = Cast<ALRGuardCharacter>(GetPawn());
	if (guard && FVector::Dist2D(guard->GetActorLocation(), target->GetActorLocation())
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
