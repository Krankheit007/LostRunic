/**
 * @file LRGuardAIControllerBehavior.cpp
 * @brief 执行 Idle、Suspicious、Investigate、Chase、Stunned 五个 StateTree 行为。
 */
#include "AI/LRGuardAIController.h"

#include "AI/LRAlertRules.h"

#include "AI/LRAlertComponent.h"
#include "AI/LRGuardCharacter.h"
#include "AI/LRGuardKnowledgeComponent.h"
#include "Core/LRGameplayTags.h"
#include "DrawDebugHelpers.h"
#include "Core/LRLog.h"
#include "Data/LRGuardTuning.h"
#include "Data/LRStateTuning.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "TimerManager.h"

ELRGuardBehaviorEntryResult ALRGuardAIController::EnterBehavior(
	const ELRGuardBehaviorState behavior)
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
		return ELRGuardBehaviorEntryResult::Running;
	}

	UCharacterMovementComponent* movement = guard->GetCharacterMovement();
	if (!movement)
	{
		return ELRGuardBehaviorEntryResult::Running;
	}

	if (behavior == ELRGuardBehaviorState::Chase)
	{
		movement->MaxWalkSpeed = GetEffectiveTuning().ChaseSpeed;
		movement->bOrientRotationToMovement = false;
		movement->bUseControllerDesiredRotation = true;
		if (AActor* threat = Knowledge.IsValid() ? Knowledge->GetSnapshot().ConfirmedThreat.Get() : nullptr)
		{
			SetFocus(threat);
			MoveToActor(threat, GetEffectiveTuning().CaptureRadius);
		}
		return ELRGuardBehaviorEntryResult::Running;
	}

	if (behavior == ELRGuardBehaviorState::Investigate)
	{
		movement->MaxWalkSpeed = GetEffectiveTuning().InvestigateSpeed;
		movement->bOrientRotationToMovement = true;
		movement->bUseControllerDesiredRotation = false;
		ClearFocus(EAIFocusPriority::Gameplay);
		if (Knowledge.IsValid())
		{
			const FLRGuardKnowledgeSnapshot knowledge = Knowledge->GetSnapshot();
			if (knowledge.bHasLatestInvestigationLocation)
			{
				RequestInvestigationMove(knowledge.LatestInvestigationLocation);
			}
		}
		return ELRGuardBehaviorEntryResult::Running;
	}

	if (behavior == ELRGuardBehaviorState::Suspicious)
	{
		StopMovement();
		movement->bOrientRotationToMovement = false;
		movement->bUseControllerDesiredRotation = true;
		if (Knowledge.IsValid())
		{
			const FLRGuardKnowledgeSnapshot knowledge = Knowledge->GetSnapshot();
			if (knowledge.bHasLatestInvestigationLocation)
			{
				SetFocalPoint(knowledge.LatestInvestigationLocation);
				CurrentSuspiciousFocusLocation = knowledge.LatestInvestigationLocation;
				bHasSuspiciousFocusLocation = true;
			}
		}
		return ELRGuardBehaviorEntryResult::Running;
	}

	if (behavior == ELRGuardBehaviorState::Stunned)
	{
		StopMovement();
		ClearFocus(EAIFocusPriority::Gameplay);
		return ELRGuardBehaviorEntryResult::Running;
	}

	movement->MaxWalkSpeed = GetEffectiveTuning().PatrolSpeed;
	movement->bOrientRotationToMovement = true;
	movement->bUseControllerDesiredRotation = false;
	ClearFocus(EAIFocusPriority::Gameplay);
	StartPatrolMove();
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

void ALRGuardAIController::OnMoveCompleted(const FAIRequestID requestId,
	const FPathFollowingResult& result)
{
	Super::OnMoveCompleted(requestId, result);

	const bool bIsCurrentInvestigationRequest = ActiveBehavior == ELRGuardBehaviorState::Investigate
		&& InvestigationMoveRequestId.IsValid()
		&& requestId.GetID() == InvestigationMoveRequestId.GetID();
	if (bIsCurrentInvestigationRequest)
	{
		if (result.IsSuccess())
		{
			SetInvestigationAtLocation();
			ProcessAwarenessTransaction(LRGameplayTags::InvestigationReached, true);
			return;
		}
		if (result.Code == EPathFollowingResult::Aborted
			&& result.HasFlag(FPathFollowingResultFlags::NewRequest))
		{
			return;
		}

		SetInvestigationAtLocation();
		ProcessAwarenessTransaction(LRGameplayTags::InvestigationUnreachable, true);
		return;
	}

	if (ActiveBehavior == ELRGuardBehaviorState::Chase && result.IsSuccess())
	{
		HandleCaptureCheck();
		return;
	}

	if (result.IsSuccess() && ActiveBehavior == ELRGuardBehaviorState::IdlePatrol)
	{
		++PatrolIndex;
		StartPatrolMove();
	}
}

void ALRGuardAIController::RefreshBehaviorContext(const FLRGuardAwarenessSnapshot& current)
{
	if (current.ResolvedBehavior == ELRGuardBehaviorState::Suspicious)
	{
		StopMovement();
		if (bHasSuspiciousFocusLocation
			&& CurrentSuspiciousFocusLocation.Equals(current.InvestigationLocation))
		{
			return;
		}
		if (current.Knowledge.bHasLatestInvestigationLocation)
		{
			SetFocalPoint(current.InvestigationLocation);
			CurrentSuspiciousFocusLocation = current.InvestigationLocation;
			bHasSuspiciousFocusLocation = true;
		}
		return;
	}

	if (current.ResolvedBehavior != ELRGuardBehaviorState::Investigate
		|| !current.Knowledge.bHasLatestInvestigationLocation)
	{
		return;
	}

	if (bSightToChaseGraceActive)
	{
		if (InvestigationMoveStatus != ELRGuardInvestigationMoveStatus::Moving)
		{
			StopMovement();
			SetFocalPoint(current.Knowledge.bHasLastKnownThreatLocation
				? current.Knowledge.LastKnownThreatLocation
				: current.InvestigationLocation);
		}
		return;
	}

	if (bForceInvestigationRetarget)
	{
		bForceInvestigationRetarget = false;
		const APawn* pawn = GetPawn();
		if (pawn && FVector::Dist2D(pawn->GetActorLocation(), current.InvestigationLocation)
			<= GetEffectiveTuning().MoveAcceptanceRadius)
		{
			SetInvestigationAtLocation();
			return;
		}
		RequestInvestigationMove(current.InvestigationLocation, true);
		return;
	}

	if (InvestigationMoveStatus == ELRGuardInvestigationMoveStatus::AtLocation)
	{
		RequestInvestigationObservationIfReady();
		return;
	}

	if (InvestigationMoveStatus == ELRGuardInvestigationMoveStatus::None)
	{
		RequestInvestigationMove(current.InvestigationLocation);
		return;
	}

	if (FVector::DistSquared(CurrentInvestigationMoveTarget, current.InvestigationLocation)
		>= FMath::Square(GetEffectiveTuning().InvestigateMoveRetargetDistanceCm))
	{
		RequestInvestigationMove(current.InvestigationLocation);
	}
}

AActor* ALRGuardAIController::GetActiveVisualCandidate() const
{
	if (!Knowledge.IsValid())
	{
		return nullptr;
	}
	const FLRGuardKnowledgeSnapshot knowledge = Knowledge->GetSnapshot();
	return knowledge.bCurrentlyVisible ? knowledge.VisualCandidate.Get() : nullptr;
}

void ALRGuardAIController::ClearInvestigationMoveRequest()
{
	InvestigationMoveRequestId = FAIRequestID::InvalidRequest;
	CurrentInvestigationMoveTarget = FVector::ZeroVector;
	InvestigationMoveStatus = ELRGuardInvestigationMoveStatus::None;
}

void ALRGuardAIController::SetInvestigationAtLocation()
{
	StopMovement();
	InvestigationMoveRequestId = FAIRequestID::InvalidRequest;
	InvestigationMoveStatus = ELRGuardInvestigationMoveStatus::AtLocation;
	RequestInvestigationObservationIfReady();
}

void ALRGuardAIController::RequestInvestigationObservationIfReady()
{
	if (!Alert.IsValid() || bSightToChaseGraceActive)
	{
		return;
	}

	const int32 alertLevel = Alert->GetAlertLevel();
	if (alertLevel >= LRAlertRules::InvestigateMinLevel
		&& alertLevel <= LRAlertRules::InvestigateMaxLevel)
	{
		Alert->StartRedObservation();
		if (Knowledge.IsValid())
		{
			const FLRGuardKnowledgeSnapshot knowledge = Knowledge->GetSnapshot();
			if (knowledge.bHasLatestInvestigationLocation)
			{
				SetFocalPoint(knowledge.LatestInvestigationLocation);
			}
		}
	}
}

FPathFollowingRequestResult ALRGuardAIController::RequestInvestigationMove(
	const FVector& location, const bool bForceRetarget)
{
	FPathFollowingRequestResult result;
	if (!GetPawn() || !Alert.IsValid() || !Knowledge.IsValid())
	{
		return result;
	}

	if (!bForceRetarget
		&& InvestigationMoveStatus == ELRGuardInvestigationMoveStatus::Moving
		&& FVector::DistSquared(CurrentInvestigationMoveTarget, location)
			< FMath::Square(GetEffectiveTuning().InvestigateMoveRetargetDistanceCm))
	{
		result.Code = EPathFollowingRequestResult::RequestSuccessful;
		result.MoveId = InvestigationMoveRequestId;
		return result;
	}

	if (!bForceRetarget
		&& InvestigationMoveStatus == ELRGuardInvestigationMoveStatus::AtLocation
		&& FVector::Dist2D(GetPawn()->GetActorLocation(), location)
			<= GetEffectiveTuning().MoveAcceptanceRadius)
	{
		result.Code = EPathFollowingRequestResult::AlreadyAtGoal;
		SetInvestigationAtLocation();
		return result;
	}

	ClearInvestigationMoveRequest();
	FAIMoveRequest moveRequest(location);
	moveRequest.SetAcceptanceRadius(GetEffectiveTuning().MoveAcceptanceRadius);
	result = MoveTo(moveRequest);
	++InvestigationMoveRequestCount;
	CurrentInvestigationMoveTarget = location;

	if (result.Code == EPathFollowingRequestResult::RequestSuccessful)
	{
		InvestigationMoveStatus = ELRGuardInvestigationMoveStatus::Moving;
		InvestigationMoveRequestId = result.MoveId;
		return result;
	}

	InvestigationMoveRequestId = FAIRequestID::InvalidRequest;
	InvestigationMoveStatus = ELRGuardInvestigationMoveStatus::AtLocation;
	if (result.Code == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		RequestInvestigationObservationIfReady();
	}
	else
	{
		UE_LOG(LogLostRunicAI, Warning,
			TEXT("Controller=%s investigation navigation failed; observing target=%s"),
			*GetNameSafe(this), *location.ToCompactString());
		RequestInvestigationObservationIfReady();
	}
	return result;
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
	if (!patrolPoint)
	{
		StopMovement();
		return;
	}

	const EPathFollowingRequestResult::Type result = MoveToActor(
		patrolPoint, GetEffectiveTuning().MoveAcceptanceRadius);
	if (result == EPathFollowingRequestResult::Failed)
	{
		UE_LOG(LogLostRunicAI, Warning,
			TEXT("Controller=%s Pawn=%s patrol MoveTo failed target=%s"),
			*GetNameSafe(this), *GetNameSafe(guard), *GetNameSafe(patrolPoint));
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
	UE_LOG(LogLostRunicAI, Display,
		TEXT("Guard=%s Alert=%d Behavior=%d Visible=%d ConfirmedThreat=%s Reason=%s Location=%s"),
		*GetNameSafe(guard), awareness.Alert.Level, static_cast<int32>(awareness.ResolvedBehavior),
		awareness.Knowledge.bCurrentlyVisible ? 1 : 0,
		*GetNameSafe(awareness.Knowledge.ConfirmedThreat.Get()),
		*Alert->GetLastReason().GetTagName().ToString(),
		*awareness.InvestigationLocation.ToCompactString());

	const FVector origin = guard->GetActorLocation();
	const UAISenseConfig_Sight* sight = AIPerception
		? AIPerception->GetSenseConfig<UAISenseConfig_Sight>() : nullptr;
	const UAISenseConfig_Hearing* hearing = AIPerception
		? AIPerception->GetSenseConfig<UAISenseConfig_Hearing>() : nullptr;
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
	DrawDebugSphere(GetWorld(), origin, GetEffectiveTuning().CaptureRadius, 16, FColor::Red, false, 5.0f);
}
