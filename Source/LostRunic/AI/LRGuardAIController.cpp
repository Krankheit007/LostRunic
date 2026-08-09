#include "AI/LRGuardAIController.h"

#include "AI/LRAlertComponent.h"
#include "AI/LRGuardCharacter.h"
#include "AI/LRGuardPerceptionRules.h"
#include "Components/ActorComponent.h"
#include "Components/StateTreeAIComponent.h"
#include "Core/LRGameplayTags.h"
#include "Core/LRLog.h"
#include "Data/LRGameTuningSet.h"
#include "Data/LRGuardTuning.h"
#include "DrawDebugHelpers.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Stealth/LRGuardVisibility.h"
#include "TimerManager.h"

ALRGuardAIController::ALRGuardAIController()
{
	PrimaryActorTick.bCanEverTick = false;
	bStartAILogicOnPossess = true;
	bStopAILogicOnUnposses = true;
	bAttachToPawn = true;
	StateTreeAI = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAI"));
	AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
	SetPerceptionComponent(*AIPerception);
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
}

void ALRGuardAIController::BeginPlay()
{
	Super::BeginPlay();
	const UGameInstance* gameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const ULRGameInstanceSubsystem* subsystem = gameInstance ? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	Tuning = subsystem && subsystem->GetTuningSet() ? subsystem->GetTuningSet()->Guard : nullptr;
	if (!ensureMsgf(Tuning, TEXT("%s requires Guard tuning."), *GetNameSafe(this)))
	{
		return;
	}
	ConfigurePerception();
	AIPerception->OnTargetPerceptionUpdated.AddDynamic(this, &ALRGuardAIController::HandlePerception);
	GetWorld()->GetTimerManager().SetTimer(CaptureTimer, this, &ALRGuardAIController::HandleCaptureTimer,
		Tuning->CaptureCheckIntervalSeconds, true);
}

void ALRGuardAIController::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	if (AIPerception)
	{
		AIPerception->OnTargetPerceptionUpdated.RemoveDynamic(this, &ALRGuardAIController::HandlePerception);
	}
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(CaptureTimer);
		GetWorld()->GetTimerManager().ClearTimer(SearchTimer);
	}
	Super::EndPlay(endPlayReason);
}

void ALRGuardAIController::OnPossess(APawn* inPawn)
{
	Super::OnPossess(inPawn);
	ALRGuardCharacter* guard = Cast<ALRGuardCharacter>(inPawn);
	Alert = guard ? guard->GetAlertComponent() : nullptr;
	if (Alert.IsValid())
	{
		Alert->OnAlertChanged.AddDynamic(this, &ALRGuardAIController::HandleAlertChanged);
	}
}

void ALRGuardAIController::OnUnPossess()
{
	if (Alert.IsValid())
	{
		Alert->OnAlertChanged.RemoveDynamic(this, &ALRGuardAIController::HandleAlertChanged);
	}
	Alert.Reset();
	Super::OnUnPossess();
}

void ALRGuardAIController::EnterBehavior(const ELRGuardBehaviorState behavior)
{
	ActiveBehavior = behavior;
	ALRGuardCharacter* guard = Cast<ALRGuardCharacter>(GetPawn());
	if (!guard || !Alert.IsValid())
	{
		return;
	}
	GetWorld()->GetTimerManager().ClearTimer(SearchTimer);
	UCharacterMovementComponent* movement = guard->GetCharacterMovement();
	if (behavior == ELRGuardBehaviorState::Chase)
	{
		movement->MaxWalkSpeed = GetEffectiveTuning().ChaseSpeed;
		SetFocus(Alert->GetTargetActor());
		MoveToActor(Alert->GetTargetActor(), GetEffectiveTuning().CaptureRadius);
	}
	else if (behavior == ELRGuardBehaviorState::Investigate)
	{
		movement->MaxWalkSpeed = GetEffectiveTuning().InvestigateSpeed;
		MoveToLocation(Alert->GetLastDisturbanceLocation(), GetEffectiveTuning().MoveAcceptanceRadius);
	}
	else if (behavior == ELRGuardBehaviorState::Search)
	{
		StopMovement();
		SetFocalPoint(Alert->GetLastDisturbanceLocation());
		GetWorld()->GetTimerManager().SetTimer(SearchTimer, this, &ALRGuardAIController::HandleSearchTimeout,
			GetEffectiveTuning().SearchDurationSeconds, false);
	}
	else if (behavior == ELRGuardBehaviorState::Suspicious)
	{
		StopMovement();
		SetFocalPoint(Alert->GetLastDisturbanceLocation());
	}
	else
	{
		movement->MaxWalkSpeed = GetEffectiveTuning().InvestigateSpeed;
		StartPatrolMove();
	}
}

void ALRGuardAIController::ExitBehavior(const ELRGuardBehaviorState behavior)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(SearchTimer);
	}
	StopMovement();
	ClearFocus(EAIFocusPriority::Gameplay);
}

void ALRGuardAIController::OnMoveCompleted(const FAIRequestID requestId, const FPathFollowingResult& result)
{
	Super::OnMoveCompleted(requestId, result);
	if (!result.IsSuccess() || !Alert.IsValid())
	{
		return;
	}
	if (ActiveBehavior == ELRGuardBehaviorState::Investigate)
	{
		Alert->MarkInvestigationReached();
	}
	else if (ActiveBehavior == ELRGuardBehaviorState::IdlePatrol)
	{
		++PatrolIndex;
		StartPatrolMove();
	}
}

void ALRGuardAIController::HandlePerception(AActor* actor, const FAIStimulus stimulus)
{
	if (!actor || !Alert.IsValid())
	{
		return;
	}
	if (stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
	{
		const bool bVisible = stimulus.WasSuccessfullySensed() && CanConfirmSight(actor);
		Alert->SetSightTarget(actor, bVisible, stimulus.StimulusLocation);
	}
	else if (stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>() && stimulus.WasSuccessfullySensed())
	{
		FGameplayTag reason = FGameplayTag::RequestGameplayTag(stimulus.Tag, false);
		if (!reason.IsValid())
		{
			reason = LRGameplayTags::NoiseInteraction;
		}
		Alert->ApplyAlertDelta(GetEffectiveTuning().HearingAlertAmount, stimulus.StimulusLocation, actor, reason);
	}
}

void ALRGuardAIController::HandleAlertChanged(const int32 previousLevel, const int32 currentLevel,
	const ELRGuardBehaviorState currentState, const FGameplayTag reason, const FVector disturbanceLocation)
{
	StateTreeAI->SendStateTreeEvent(LRGameplayTags::AIEventAlertChanged, FConstStructView(), reason.GetTagName());
	if (!StateTreeAI->IsRunning())
	{
		EnterBehavior(currentState);
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
	if (!Alert.IsValid() || Alert->GetBehaviorState() != ELRGuardBehaviorState::Chase)
	{
		return;
	}
	AActor* target = Alert->GetTargetActor();
	if (!CanConfirmSight(target))
	{
		Alert->SetSightTarget(target, false, target ? target->GetActorLocation() : FVector::ZeroVector);
		return;
	}
	ALRGuardCharacter* guard = Cast<ALRGuardCharacter>(GetPawn());
	if (guard && FVector::Dist2D(guard->GetActorLocation(), target->GetActorLocation()) <= GetEffectiveTuning().CaptureRadius)
	{
		guard->CaptureTarget(target);
	}
}

void ALRGuardAIController::HandleSearchTimeout()
{
	if (Alert.IsValid())
	{
		Alert->ResetAfterSearch();
	}
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

bool ALRGuardAIController::CanConfirmSight(AActor* actor) const
{
	const APawn* guardPawn = GetPawn();
	if (!actor || !guardPawn)
	{
		return false;
	}
	const FVector toTarget = actor->GetActorLocation() - guardPawn->GetActorLocation();
	const float distance = toTarget.Size2D();
	const float forwardDot = FVector::DotProduct(guardPawn->GetActorForwardVector().GetSafeNormal2D(),
		toTarget.GetSafeNormal2D());
	return LRGuardPerceptionRules::CanConfirmSight(distance, forwardDot, !LineOfSightTo(actor),
		IsHiddenFromGuard(actor), GetEffectiveTuning());
}

bool ALRGuardAIController::IsHiddenFromGuard(AActor* actor) const
{
	if (actor->GetClass()->ImplementsInterface(ULRGuardVisibility::StaticClass()))
	{
		return !ILRGuardVisibility::Execute_IsVisibleToGuard(actor, const_cast<ALRGuardAIController*>(this));
	}
	for (UActorComponent* component : actor->GetComponents())
	{
		if (component && component->GetClass()->ImplementsInterface(ULRGuardVisibility::StaticClass())
			&& !ILRGuardVisibility::Execute_IsVisibleToGuard(component, const_cast<ALRGuardAIController*>(this)))
		{
			return true;
		}
	}
	return false;
}

void ALRGuardAIController::LogAndDrawDiagnostics() const
{
	const APawn* guard = GetPawn();
	if (!guard || !Alert.IsValid())
	{
		return;
	}
	const ULRGuardTuning& tuning = GetEffectiveTuning();
	UE_LOG(LogLostRunicAI, Display, TEXT("Guard=%s Alert=%d State=%d Target=%s Reason=%s Location=%s"),
		*GetNameSafe(guard), Alert->GetAlertLevel(), static_cast<int32>(Alert->GetBehaviorState()),
		*GetNameSafe(Alert->GetTargetActor()), *Alert->GetLastReason().ToString(),
		*Alert->GetLastDisturbanceLocation().ToCompactString());
	const FVector origin = guard->GetActorLocation();
	DrawDebugCone(GetWorld(), origin, guard->GetActorForwardVector(), tuning.SightRadius,
		FMath::DegreesToRadians(tuning.SightConeDegrees * 0.5f), FMath::DegreesToRadians(tuning.SightConeDegrees * 0.5f),
		16, FColor::Yellow, false, 5.0f);
	DrawDebugSphere(GetWorld(), origin, tuning.MaxHearingRange, 32, FColor::Cyan, false, 5.0f);
	DrawDebugSphere(GetWorld(), origin, tuning.CaptureRadius, 16, FColor::Red, false, 5.0f);
}

const ULRGuardTuning& ALRGuardAIController::GetEffectiveTuning() const
{
	return Tuning ? *Tuning : *GetDefault<ULRGuardTuning>();
}
