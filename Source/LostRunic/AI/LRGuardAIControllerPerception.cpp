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
			const FLRGuardAwarenessSnapshot previous = GetAwarenessSnapshot();
			PerceivedSightContact = actor;
			Knowledge->SetVisualCandidate(actor);
			LastDetectionSampleTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
			CommitAwareness(previous, Alert->GetAlertLevel(), LRGameplayTags::SightPlayer, true);
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
	AActor* candidate = PerceivedSightContact.Get();
	if (!IsRelevantSightTarget(candidate))
	{
		if (Knowledge->HasVisualCandidate())
		{
			HandleSightLost(candidate, Knowledge->GetLastKnownThreatLocation());
		}
		return;
	}

	const double now = GetWorld()->GetTimeSeconds();
	const float deltaSeconds = LastDetectionSampleTime > 0.0
		? static_cast<float>(now - LastDetectionSampleTime) : GetEffectiveTuning().DetectionSampleIntervalSeconds;
	LastDetectionSampleTime = now;
	const FLRGuardAwarenessSnapshot previous = GetAwarenessSnapshot();
	const int32 previousLevel = Alert->GetAlertLevel();
	const ELRGuardDetectionStage previousStage = previous.Knowledge.Stage;
	const FLRGuardVisibilityResult visibility = EvaluateVisibility(candidate);
	Knowledge->ApplyVisibilitySample(visibility, deltaSeconds, GetEffectiveTuning());
	const ELRGuardDetectionStage currentStage = Knowledge->GetDetectionStage();

	if (!visibility.IsActive() && previous.Knowledge.CurrentVisibility.IsActive())
	{
		Knowledge->RecordSightLoss(candidate->GetActorLocation());
		if (previousLevel >= GetEffectiveTuning().SightChaseLevel)
		{
			Alert->LowerToMaximum(GetEffectiveTuning().SightChaseLevel - 1);
		}
		CommitAwareness(previous, previousLevel, LRGameplayTags::SightPlayerLost, true);
		return;
	}
	if (visibility.IsActive())
	{
		if (currentStage == ELRGuardDetectionStage::Confirmed)
		{
			Knowledge->SetConfirmedThreat(candidate, candidate->GetActorLocation());
			Alert->RaiseToMinimum(GetEffectiveTuning().SightChaseLevel);
		}
		else if (static_cast<uint8>(currentStage) >= static_cast<uint8>(ELRGuardDetectionStage::Investigate))
		{
			Knowledge->RecordVisualEvidence(candidate, candidate->GetActorLocation(), true,
				GetEffectiveTuning().InvestigationRetargetDistance);
		}
		else if (currentStage == ELRGuardDetectionStage::Suspicious)
		{
			Knowledge->RecordVisualEvidence(candidate, candidate->GetActorLocation(), false,
				GetEffectiveTuning().InvestigationRetargetDistance);
		}
	}
	if (static_cast<uint8>(currentStage) > static_cast<uint8>(previousStage))
	{
		int32 floor = 0;
		if (currentStage == ELRGuardDetectionStage::Suspicious)
		{
			floor = 1;
		}
		else if (currentStage == ELRGuardDetectionStage::Investigate)
		{
			floor = GetEffectiveTuning().SightInvestigateLevel;
		}
		else if (currentStage == ELRGuardDetectionStage::Confirmed)
		{
			floor = GetEffectiveTuning().SightChaseLevel;
		}
		Alert->RaiseToMinimum(floor);
	}
	CommitAwareness(previous, previousLevel, LRGameplayTags::SightPlayer);
}

void ALRGuardAIController::HandleSightLost(AActor* actor, const FVector& lastKnownLocation)
{
	if (!Alert.IsValid() || !Knowledge.IsValid())
	{
		return;
	}
	const FLRGuardAwarenessSnapshot previous = GetAwarenessSnapshot();
	const int32 previousLevel = Alert->GetAlertLevel();
	Knowledge->RecordSightLoss(lastKnownLocation);
	if (previousLevel >= GetEffectiveTuning().SightChaseLevel)
	{
		Alert->LowerToMaximum(GetEffectiveTuning().SightChaseLevel - 1);
	}
	PerceivedSightContact.Reset();
	LastDetectionSampleTime = 0.0;
	CommitAwareness(previous, previousLevel, LRGameplayTags::SightPlayerLost, true);
}

FLRGuardVisibilityResult ALRGuardAIController::EvaluateVisibility(AActor* actor) const
{
	const APawn* guardPawn = GetPawn();
	if (!actor || !guardPawn)
	{
		return FLRGuardVisibilityResult();
	}
	const FVector toTarget = actor->GetActorLocation() - guardPawn->GetActorLocation();
	const float distance = toTarget.Size2D();
	const float forwardDot = FVector::DotProduct(guardPawn->GetActorForwardVector().GetSafeNormal2D(),
		toTarget.GetSafeNormal2D());
	return LRGuardPerceptionRules::EvaluateVisibility(distance, forwardDot,
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

void ALRGuardAIController::ConfigurePerception()
{
	const ULRGuardTuning& tuning = GetEffectiveTuning();
	SightConfig->SightRadius = tuning.SightRadius;
	SightConfig->LoseSightRadius = tuning.LoseSightRadius;
	SightConfig->PeripheralVisionAngleDegrees = tuning.SightConeDegrees * 0.5f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	HearingConfig->HearingRange = tuning.MaxHearingRange * tuning.HearingRangeMultiplier;
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	AIPerception->ConfigureSense(*SightConfig);
	AIPerception->ConfigureSense(*HearingConfig);
	AIPerception->SetDominantSense(SightConfig->GetSenseImplementation());
}

void ALRGuardAIController::HandleCaptureTimer()
{
	const FLRGuardAwarenessSnapshot awareness = GetAwarenessSnapshot();
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
