/**
 * @file LRGuardAIControllerBehavior.cpp
 * @brief Executes resolved behavior and refreshes behavior context without inventing state transitions.
 */
#include "AI/LRGuardAIController.h"

#include "AI/LRAlertComponent.h"
#include "AI/LRGuardCharacter.h"
#include "Components/StateTreeAIComponent.h"
#include "Core/LRGameplayTags.h"
#include "Core/LRLog.h"
#include "Data/LRGuardTuning.h"
#include "Data/LRStateTuning.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "TimerManager.h"

void ALRGuardAIController::EnterBehavior(const ELRGuardBehaviorState behavior)
{
	if (bStunned && behavior != ELRGuardBehaviorState::Stunned)
	{
		ActiveBehavior = ELRGuardBehaviorState::Stunned;
		return;
	}
	ActiveBehavior = behavior;
	ALRGuardCharacter* guard = Cast<ALRGuardCharacter>(GetPawn());
	if (!guard)
	{
		return;
	}
	const FLRGuardAwarenessSnapshot awareness = GetAwarenessSnapshot();
	UCharacterMovementComponent* movement = guard->GetCharacterMovement();
	if (behavior == ELRGuardBehaviorState::Chase)
	{
		AActor* threat = awareness.Knowledge.ConfirmedThreat.Get();
		movement->MaxWalkSpeed = GetEffectiveTuning().ChaseSpeed;
		SetFocus(threat);
		MoveToActor(threat, GetEffectiveTuning().CaptureRadius);
	}
	else if (behavior == ELRGuardBehaviorState::Investigate)
	{
		movement->MaxWalkSpeed = GetEffectiveTuning().InvestigateSpeed;
		MoveToLocation(awareness.InvestigationLocation, GetEffectiveTuning().MoveAcceptanceRadius);
	}
	else if (behavior == ELRGuardBehaviorState::Search)
	{
		StopMovement();
		SetFocalPoint(awareness.InvestigationLocation);
	}
	else if (behavior == ELRGuardBehaviorState::Suspicious)
	{
		StopMovement();
		SetFocalPoint(awareness.InvestigationLocation);
	}
	else if (behavior == ELRGuardBehaviorState::Stunned)
	{
		StopMovement();
		ClearFocus(EAIFocusPriority::Gameplay);
	}
	else
	{
		movement->MaxWalkSpeed = GetEffectiveTuning().InvestigateSpeed;
		StartPatrolMove();
	}
}

void ALRGuardAIController::ExitBehavior(const ELRGuardBehaviorState behavior)
{
	StopMovement();
	ClearFocus(EAIFocusPriority::Gameplay);
}

void ALRGuardAIController::OnMoveCompleted(const FAIRequestID requestId, const FPathFollowingResult& result)
{
	Super::OnMoveCompleted(requestId, result);
	if (!result.IsSuccess())
	{
		return;
	}
	if (ActiveBehavior == ELRGuardBehaviorState::Investigate)
	{
		MarkInvestigationReached();
	}
	else if (ActiveBehavior == ELRGuardBehaviorState::IdlePatrol)
	{
		++PatrolIndex;
		StartPatrolMove();
	}
}

void ALRGuardAIController::RefreshBehaviorContext(const FLRGuardAwarenessSnapshot& previous,
	const FLRGuardAwarenessSnapshot& current)
{
	if (current.ResolvedBehavior != ELRGuardBehaviorState::Investigate
		&& current.ResolvedBehavior != ELRGuardBehaviorState::Suspicious)
	{
		return;
	}
	if (previous.Knowledge.InvestigationContextRevision
		== current.Knowledge.InvestigationContextRevision)
	{
		return;
	}
	if (FVector::DistSquared(previous.InvestigationLocation, current.InvestigationLocation)
		< FMath::Square(GetEffectiveTuning().InvestigationRetargetDistance))
	{
		return;
	}
	if (current.ResolvedBehavior == ELRGuardBehaviorState::Investigate)
	{
		MoveToLocation(current.InvestigationLocation, GetEffectiveTuning().MoveAcceptanceRadius);
	}
	else
	{
		SetFocalPoint(current.InvestigationLocation);
	}
}

void ALRGuardAIController::HandleKnockback(const FVector direction)
{
	if (bStunned)
	{
		return;
	}
	const FLRGuardAwarenessSnapshot previous = GetAwarenessSnapshot();
	const int32 previousLevel = Alert.IsValid() ? Alert->GetAlertLevel() : 0;
	bStunned = true;
	StopMovement();
	ClearFocus(EAIFocusPriority::Gameplay);
	UE_LOG(LogLostRunicAI, Display, TEXT("Guard=%s stunned for %.2fs direction=%s"), *GetNameSafe(GetPawn()),
		StateTuning ? StateTuning->CourageKnockbackDurationSeconds : 0.6f, *direction.ToCompactString());
	CommitAwareness(previous, previousLevel, LRGameplayTags::TargetGuardCourageVulnerable, true);
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(StunTimer, this, &ALRGuardAIController::HandleStunEnd,
			StateTuning ? StateTuning->CourageKnockbackDurationSeconds : 0.6f, false);
	}
}

void ALRGuardAIController::HandleStunEnd()
{
	const FLRGuardAwarenessSnapshot previous = GetAwarenessSnapshot();
	const int32 previousLevel = Alert.IsValid() ? Alert->GetAlertLevel() : 0;
	bStunned = false;
	CommitAwareness(previous, previousLevel, LRGameplayTags::TargetGuardCourageVulnerable, true);
}

void ALRGuardAIController::StartPatrolMove()
{
	ALRGuardCharacter* guard = Cast<ALRGuardCharacter>(GetPawn());
	if (!guard || guard->GetPatrolPointCount() == 0)
	{
		StopMovement();
		return;
	}
	PatrolIndex %= guard->GetPatrolPointCount();
	MoveToActor(guard->GetPatrolPoint(PatrolIndex), GetEffectiveTuning().MoveAcceptanceRadius);
}

void ALRGuardAIController::LogAndDrawDiagnostics() const
{
	const APawn* guard = GetPawn();
	if (!guard || !Alert.IsValid())
	{
		return;
	}
	const FLRGuardAwarenessSnapshot awareness = GetAwarenessSnapshot();
	const ULRGuardTuning& tuning = GetEffectiveTuning();
	UE_LOG(LogLostRunicAI, Display,
		TEXT("Guard=%s Alert=%d Behavior=%d Stage=%d Visible=%d Threat=%s Reason=%s Location=%s"),
		*GetNameSafe(guard), awareness.Alert.Level, static_cast<int32>(awareness.ResolvedBehavior),
		static_cast<int32>(awareness.Knowledge.Stage), awareness.Knowledge.CurrentVisibility.IsActive() ? 1 : 0,
		*GetNameSafe(awareness.Knowledge.ConfirmedThreat.Get()),
		*Alert->GetLastReason().GetTagName().ToString(), *awareness.InvestigationLocation.ToCompactString());
	const FVector origin = guard->GetActorLocation();
	DrawDebugCone(GetWorld(), origin, guard->GetActorForwardVector(), tuning.SightRadius,
		FMath::DegreesToRadians(tuning.SightConeDegrees * 0.5f),
		FMath::DegreesToRadians(tuning.SightConeDegrees * 0.5f), 16, FColor::Yellow, false, 5.0f);
	DrawDebugSphere(GetWorld(), origin, tuning.MaxHearingRange, 32, FColor::Cyan, false, 5.0f);
	DrawDebugSphere(GetWorld(), origin, tuning.CaptureRadius, 16, FColor::Red, false, 5.0f);
}
