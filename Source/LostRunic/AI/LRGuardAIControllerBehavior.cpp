/**
 * @file LRGuardAIControllerBehavior.cpp
 * @brief Executes resolved behavior and refreshes behavior context without inventing state transitions.
 */
#include "AI/LRGuardAIController.h"

#include "AI/LRAlertComponent.h"
#include "AI/LRGuardCharacter.h"
#include "AI/LRGuardKnowledgeComponent.h"
#include "Components/StateTreeAIComponent.h"
#include "Core/LRGameplayTags.h"
#include "Core/LRLog.h"
#include "Data/LRGuardTuning.h"
#include "Data/LRStateTuning.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "TimerManager.h"

ELRGuardBehaviorEntryResult ALRGuardAIController::EnterBehavior(const ELRGuardBehaviorState behavior)
{
	if (bStunned && behavior != ELRGuardBehaviorState::Stunned)
	{
		ActiveBehavior = ELRGuardBehaviorState::Stunned;
		return ELRGuardBehaviorEntryResult::Running;
	}
	ActiveBehavior = behavior;
	ALRGuardCharacter* guard = Cast<ALRGuardCharacter>(GetPawn());
	if (!guard)
	{
		return behavior == ELRGuardBehaviorState::Investigate
			? ELRGuardBehaviorEntryResult::Failed : ELRGuardBehaviorEntryResult::Running;
	}
	const FLRGuardAwarenessSnapshot awareness = BuildCurrentAwarenessSnapshot();
	UCharacterMovementComponent* movement = guard->GetCharacterMovement();
	if (behavior == ELRGuardBehaviorState::Chase)
	{
		AActor* threat = awareness.Knowledge.ConfirmedThreat.Get();
		movement->MaxWalkSpeed = GetEffectiveTuning().ChaseSpeed;
		if (threat)
		{
			SetFocus(threat);
			MoveToActor(threat, GetEffectiveTuning().CaptureRadius);
		}
	}
	else if (behavior == ELRGuardBehaviorState::Investigate)
	{
		movement->MaxWalkSpeed = GetEffectiveTuning().InvestigateSpeed;
		if (AActor* visualCandidate = GetActiveVisualCandidate())
		{
			StopMovement();
			ClearInvestigationMoveRequest();
			SetFocus(visualCandidate);
			return ELRGuardBehaviorEntryResult::Running;
		}
		const FPathFollowingRequestResult result = RequestInvestigationMove(awareness.InvestigationLocation);
		if (result.Code == EPathFollowingRequestResult::AlreadyAtGoal)
		{
			if (AActor* visualCandidate = GetActiveVisualCandidate())
			{
				SetFocus(visualCandidate);
				return ELRGuardBehaviorEntryResult::Running;
			}
			return ELRGuardBehaviorEntryResult::AlreadyAtGoal;
		}
		if (result.Code != EPathFollowingRequestResult::RequestSuccessful)
		{
			return ELRGuardBehaviorEntryResult::Failed;
		}
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
		CurrentSuspiciousFocusLocation = awareness.InvestigationLocation;
		bHasSuspiciousFocusLocation = true;
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
	return ELRGuardBehaviorEntryResult::Running;
}
void ALRGuardAIController::ExitBehavior(const ELRGuardBehaviorState behavior)
{
	if (behavior == ELRGuardBehaviorState::Investigate)
	{
		ClearInvestigationMoveRequest();
	}
	if (behavior == ELRGuardBehaviorState::Suspicious)
	{
		bHasSuspiciousFocusLocation = false;
	}
	StopMovement();
	ClearFocus(EAIFocusPriority::Gameplay);
}
void ALRGuardAIController::OnMoveCompleted(const FAIRequestID requestId, const FPathFollowingResult& result)
{
	Super::OnMoveCompleted(requestId, result);
	const bool bIsCurrentInvestigationRequest = ActiveBehavior == ELRGuardBehaviorState::Investigate
		&& InvestigationMoveRequestId.IsValid()
		&& requestId.GetID() == InvestigationMoveRequestId.GetID();
	if (bIsCurrentInvestigationRequest)
	{
		if (result.IsSuccess())
		{
			if (AActor* visualCandidate = GetActiveVisualCandidate())
			{
				ClearInvestigationMoveRequest();
				SetFocus(visualCandidate);
				return;
			}
			MarkInvestigationReached();
			return;
		}
		if (result.Code == EPathFollowingResult::Blocked || result.Code == EPathFollowingResult::OffPath)
		{
			MarkInvestigationUnreachable();
			return;
		}
		if (result.Code == EPathFollowingResult::Aborted
			&& result.HasFlag(FPathFollowingResultFlags::NewRequest))
		{
			return;
		}
		MarkInvestigationUnreachable();
		return;
	}
	if (ActiveBehavior == ELRGuardBehaviorState::Chase && result.IsSuccess())
	{
		const FLRGuardAwarenessSnapshot awareness = BuildCurrentAwarenessSnapshot();
		AActor* threat = awareness.Knowledge.ConfirmedThreat.Get();
		ALRGuardCharacter* guard = Cast<ALRGuardCharacter>(GetPawn());
		if (guard && IsValid(threat)
			&& FVector::Dist2D(guard->GetActorLocation(), threat->GetActorLocation())
			<= GetEffectiveTuning().CaptureRadius)
		{
			guard->CaptureTarget(threat);
		}
		return;
	}
	if (!result.IsSuccess())
	{
		return;
	}
	if (ActiveBehavior == ELRGuardBehaviorState::IdlePatrol)
	{
		++PatrolIndex;
		StartPatrolMove();
	}
}
void ALRGuardAIController::RefreshBehaviorContext(const FLRGuardAwarenessSnapshot& current)
{
	if (current.ResolvedBehavior == ELRGuardBehaviorState::Investigate)
	{
		if (AActor* visualCandidate = GetActiveVisualCandidate())
		{
			StopMovement();
			ClearInvestigationMoveRequest();
			SetFocus(visualCandidate);
			return;
		}
		if (!current.Knowledge.bHasLastKnownThreatLocation
			&& !current.Knowledge.bHasLastDisturbanceLocation)
		{
			return;
		}
		const bool bHasValidRequest = bHasInvestigationMoveTarget && InvestigationMoveRequestId.IsValid();
		if (bHasValidRequest
			&& FVector::DistSquared(CurrentInvestigationMoveTarget, current.InvestigationLocation)
			< FMath::Square(GetEffectiveTuning().InvestigationRetargetDistance))
		{
			return;
		}
		RequestInvestigationMove(current.InvestigationLocation);
		return;
	}
	if (current.ResolvedBehavior == ELRGuardBehaviorState::Suspicious)
	{
		if (bHasSuspiciousFocusLocation
			&& FVector::DistSquared(CurrentSuspiciousFocusLocation, current.InvestigationLocation)
			< FMath::Square(GetEffectiveTuning().InvestigationRetargetDistance))
		{
			return;
		}
		SetFocalPoint(current.InvestigationLocation);
		CurrentSuspiciousFocusLocation = current.InvestigationLocation;
		bHasSuspiciousFocusLocation = true;
	}
}

AActor* ALRGuardAIController::GetActiveVisualCandidate() const
{
	const FLRGuardAwarenessSnapshot awareness = BuildCurrentAwarenessSnapshot();
	return awareness.Knowledge.CurrentVisibility.IsActive()
		? awareness.Knowledge.VisualCandidate.Get() : nullptr;
}

FPathFollowingRequestResult ALRGuardAIController::RequestInvestigationMove(const FVector& location)
{
	FPathFollowingRequestResult result;
	if (!GetPawn())
	{
		if (Alert.IsValid() && Knowledge.IsValid())
		{
			ApplyInvestigationUnreachable();
		}
		return result;
	}
	if (!Alert.IsValid() || !Knowledge.IsValid())
	{
		return result;
	}
	if (bHasInvestigationMoveTarget && InvestigationMoveRequestId.IsValid()
		&& CurrentInvestigationMoveTarget.Equals(location))
	{
		result.Code = EPathFollowingRequestResult::RequestSuccessful;
		result.MoveId = InvestigationMoveRequestId;
		return result;
	}
	if (!ShouldRetryInvestigationAt(location))
	{
		ApplyInvestigationUnreachable();
		return result;
	}
	ClearInvestigationRetrySuppression();
	ClearInvestigationMoveRequest();
	FAIMoveRequest moveRequest(location);
	moveRequest.SetAcceptanceRadius(GetEffectiveTuning().MoveAcceptanceRadius);
	result = MoveTo(moveRequest);
	++InvestigationMoveRequestCount;
	if (result.Code == EPathFollowingRequestResult::RequestSuccessful && result.MoveId.IsValid())
	{
		CurrentInvestigationMoveTarget = location;
		bHasInvestigationMoveTarget = true;
		InvestigationMoveRequestId = result.MoveId;
		Knowledge->AdvanceInvestigationContextRevision();
		return result;
	}
	ClearInvestigationMoveRequest();
	if (result.Code == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		ApplyInvestigationReached();
	}
	else
	{
		ApplyInvestigationUnreachable();
	}
	return result;
}
void ALRGuardAIController::HandleKnockback(const FVector direction)
{
	if (bStunned)
	{
		return;
	}
	bStunned = true;
	ClearInvestigationMoveRequest();
	StopMovement();
	ClearFocus(EAIFocusPriority::Gameplay);
	UE_LOG(LogLostRunicAI, Display, TEXT("Guard=%s stunned for %.2fs direction=%s"), *GetNameSafe(GetPawn()),
		StateTuning ? StateTuning->CourageKnockbackDurationSeconds : 0.6f, *direction.ToCompactString());
	ProcessAwarenessTransaction(LRGameplayTags::TargetGuardCourageVulnerable, true);
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(StunTimer, this, &ALRGuardAIController::HandleStunEnd,
			StateTuning ? StateTuning->CourageKnockbackDurationSeconds : 0.6f, false);
	}
}
void ALRGuardAIController::HandleStunEnd()
{
	bStunned = false;
	ProcessAwarenessTransaction(LRGameplayTags::TargetGuardCourageVulnerable, true);
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
	AActor* patrolPoint = guard->GetPatrolPoint(PatrolIndex);
	const EPathFollowingRequestResult::Type result = MoveToActor(
		patrolPoint, GetEffectiveTuning().MoveAcceptanceRadius);
	if (result == EPathFollowingRequestResult::Failed)
	{
		UE_LOG(LogLostRunicAI, Warning,
			TEXT("Controller=%s Pawn=%s patrol MoveTo failed target=%s pawnLocation=%s targetLocation=%s"),
			*GetNameSafe(this), *GetNameSafe(guard), *GetNameSafe(patrolPoint),
			*guard->GetActorLocation().ToCompactString(),
			patrolPoint ? *patrolPoint->GetActorLocation().ToCompactString() : TEXT("None"));
	}
}

void ALRGuardAIController::LogAndDrawDiagnostics() const
{
	const APawn* guard = GetPawn();
	if (!guard || !Alert.IsValid())
	{
		return;
	}
	const FLRGuardAwarenessSnapshot awareness = BuildCurrentAwarenessSnapshot();
	const FLRGuardTuningSettings& tuning = GetEffectiveTuning();
	UE_LOG(LogLostRunicAI, Display,
		TEXT("Guard=%s Alert=%d Behavior=%d Stage=%d Visible=%d Threat=%s Reason=%s Location=%s"),
		*GetNameSafe(guard), awareness.Alert.Level, static_cast<int32>(awareness.ResolvedBehavior),
		static_cast<int32>(awareness.Knowledge.Stage), awareness.Knowledge.CurrentVisibility.IsActive() ? 1 : 0,
		*GetNameSafe(awareness.Knowledge.ConfirmedThreat.Get()),
		*Alert->GetLastReason().GetTagName().ToString(), *awareness.InvestigationLocation.ToCompactString());
	const FVector origin = guard->GetActorLocation();
	const UAISenseConfig_Sight* sight = AIPerception->GetSenseConfig<UAISenseConfig_Sight>();
	const UAISenseConfig_Hearing* hearing = AIPerception->GetSenseConfig<UAISenseConfig_Hearing>();
	if (sight)
	{
		const float halfAngleRadians = FMath::DegreesToRadians(sight->PeripheralVisionAngleDegrees);
		DrawDebugCone(GetWorld(), origin, guard->GetActorForwardVector(), sight->SightRadius,
			halfAngleRadians, halfAngleRadians, 16, FColor::Yellow, false, 5.0f);
	}
	if (hearing)
	{
		DrawDebugSphere(GetWorld(), origin, hearing->HearingRange, 32, FColor::Cyan, false, 5.0f);
	}
	DrawDebugSphere(GetWorld(), origin, tuning.CaptureRadius, 16, FColor::Red, false, 5.0f);
}
