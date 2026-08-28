/**
 * @file LRGuardAIController.cpp
 * @brief Guard Controller 生命周期、Alert/Knowledge 绑定和三类感知入口的公共实现。
 */
#include "AI/LRGuardAIController.h"

#include "AI/LRAlertComponent.h"
#include "AI/LRAlertRules.h"
#include "AI/LRGuardCharacter.h"
#include "AI/LRGuardKnowledgeComponent.h"
#include "AI/LRGuardPerceptionTarget.h"
#include "Components/StateTreeAIComponent.h"
#include "Core/LRGameplayTags.h"
#include "Core/LRLog.h"
#include "Data/LRGameTuningSet.h"
#include "Data/LRStateTuning.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "Items/LRCourageResponseComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "TimerManager.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

ALRGuardAIController::ALRGuardAIController()
{
	// AAIController 的 Tick 负责维持 Focus/ControlRotation；玩法逻辑仍由事件和定时器驱动。
	PrimaryActorTick.bCanEverTick = true;
	bStartAILogicOnPossess = false;
	bStopAILogicOnUnposses = true;
	bAttachToPawn = true;

	StateTreeAI = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAI"));
	StateTreeAI->SetStartLogicAutomatically(false);
	AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
	AIPerception->SetAutoActivate(false);
	SetPerceptionComponent(*AIPerception);
}

void ALRGuardAIController::BeginPlay()
{
	Super::BeginPlay();

	const UGameInstance* gameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const ULRGameInstanceSubsystem* subsystem = gameInstance
		? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	if (subsystem && subsystem->GetTuningSet())
	{
		StateTuning = subsystem->GetTuningSet()->State;
	}
	CachedAwareness = BuildCurrentAwarenessSnapshot();
	TryInitializeRuntime();
}

void ALRGuardAIController::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	ShutdownRuntime();
	Super::EndPlay(endPlayReason);
}

void ALRGuardAIController::OnPossess(APawn* inPawn)
{
	Super::OnPossess(inPawn);

	ALRGuardCharacter* guard = Cast<ALRGuardCharacter>(inPawn);
	Alert = guard ? guard->GetAlertComponent() : nullptr;
	Knowledge = guard ? guard->GetKnowledgeComponent() : nullptr;
	if (guard)
	{
		if (ULRCourageResponseComponent* courage = guard->GetCourageResponseComponent())
		{
			courage->OnKnockbackApplied.AddUniqueDynamic(this, &ALRGuardAIController::HandleKnockback);
		}
	}

	RawSightContact.Reset();
	bHasRawSightContact = false;
	bSightToChaseGraceActive = false;
	bSightToChaseGraceConsumed = false;
	bForceInvestigationRetarget = false;
	InvestigationMoveRequestCount = 0;
	ClearInvestigationMoveRequest();
	bHasSuspiciousFocusLocation = false;
	CachedAwareness = BuildCurrentAwarenessSnapshot();
	TryInitializeRuntime();
}

bool ALRGuardAIController::ValidateControllerConfiguration(FString& outError,
	const bool bRequirePossessionContext) const
{
	if (!Tuning.Validate(outError))
	{
		outError = FString::Printf(TEXT("Guard tuning is invalid: %s"), *outError);
		return false;
	}
	if (!AIPerception || !StateTreeAI)
	{
		outError = TEXT("Native AIPerception and StateTreeAI components are required.");
		return false;
	}
	if (!AIPerception->GetSenseConfig<UAISenseConfig_Sight>()
		|| !AIPerception->GetSenseConfig<UAISenseConfig_Hearing>())
	{
		outError = TEXT("Inherited AIPerception must configure both Sight and Hearing.");
		return false;
	}
	if (AIPerception->GetDominantSense() != UAISense_Sight::StaticClass())
	{
		outError = FString::Printf(TEXT("Sight must be the Guard dominant sense; configured=%s."),
			*GetNameSafe(AIPerception->GetDominantSense()));
		return false;
	}
	if (bRequirePossessionContext
		&& (!Cast<ALRGuardCharacter>(GetPawn()) || !Alert.IsValid() || !Knowledge.IsValid()))
	{
		outError = TEXT("Possessed Guard, Alert and Knowledge are required.");
		return false;
	}
	return true;
}

void ALRGuardAIController::TryInitializeRuntime()
{
	if (bRuntimeInitialized || !HasActorBegunPlay() || !GetPawn())
	{
		return;
	}

	FString error;
	if (!ValidateControllerConfiguration(error, true))
	{
		UE_LOG(LogLostRunicAI, Error, TEXT("Controller=%s Pawn=%s runtime initialization rejected: %s"),
			*GetNameSafe(this), *GetNameSafe(GetPawn()), *error);
		return;
	}

	Alert->InitializeRuntime(Tuning);
	Alert->OnDecayRequested.RemoveAll(this);
	Alert->OnDecayRequested.AddUObject(this, &ALRGuardAIController::HandleAlertDecayRequested);
	AIPerception->OnTargetPerceptionUpdated.RemoveDynamic(this, &ALRGuardAIController::HandlePerception);
	AIPerception->OnTargetPerceptionUpdated.AddDynamic(this, &ALRGuardAIController::HandlePerception);
	AIPerception->Activate(true);
	if (!AIPerception->IsActive())
	{
		UE_LOG(LogLostRunicAI, Error, TEXT("Controller=%s Pawn=%s failed to activate AIPerception."),
			*GetNameSafe(this), *GetNameSafe(GetPawn()));
		ShutdownRuntime();
		return;
	}

	bRuntimeInitialized = true;
	StateTreeAI->StartLogic();
	if (!StateTreeAI->IsRunning())
	{
		UE_LOG(LogLostRunicAI, Error, TEXT("Controller=%s Pawn=%s failed to start configured StateTree."),
			*GetNameSafe(this), *GetNameSafe(GetPawn()));
		ShutdownRuntime();
	}
}

void ALRGuardAIController::ShutdownRuntime()
{
	if (AIPerception)
	{
		AIPerception->OnTargetPerceptionUpdated.RemoveDynamic(this, &ALRGuardAIController::HandlePerception);
		AIPerception->ForgetAll();
		AIPerception->Deactivate();
	}
	if (StateTreeAI && StateTreeAI->IsRunning())
	{
		StateTreeAI->StopLogic(TEXT("Guard runtime shutdown"));
	}

	StopSightTracking();
	StopSightToChaseGrace();
	StopMovement();
	ClearFocus(EAIFocusPriority::Gameplay);
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(StunTimer);
	}
	if (Alert.IsValid())
	{
		Alert->OnDecayRequested.RemoveAll(this);
		Alert->ShutdownRuntime();
	}
	bRuntimeInitialized = false;
}

void ALRGuardAIController::OnUnPossess()
{
	ALRGuardCharacter* guard = Cast<ALRGuardCharacter>(GetPawn());
	ShutdownRuntime();

	if (guard)
	{
		if (ULRCourageResponseComponent* courage = guard->GetCourageResponseComponent())
		{
			courage->OnKnockbackApplied.RemoveDynamic(this, &ALRGuardAIController::HandleKnockback);
		}
	}
	if (Knowledge.IsValid())
	{
		Knowledge->SuspendVisualContact();
	}

	RawSightContact.Reset();
	bHasRawSightContact = false;
	bSightToChaseGraceActive = false;
	bSightToChaseGraceConsumed = false;
	bForceInvestigationRetarget = false;
	ClearInvestigationMoveRequest();
	bHasSuspiciousFocusLocation = false;
	bAwarenessCommitDeferred = false;
	bDeferredForcePublish = false;
	DeferredAwarenessReason = FGameplayTag();
	Alert.Reset();
	Knowledge.Reset();
	CachedAwareness = FLRGuardAwarenessSnapshot();
	Super::OnUnPossess();
}

#if WITH_EDITOR
EDataValidationResult ALRGuardAIController::IsDataValid(FDataValidationContext& context) const
{
	FString error;
	if (!ValidateControllerConfiguration(error, false))
	{
		context.AddError(FText::FromString(error));
		return EDataValidationResult::Invalid;
	}
	return Super::IsDataValid(context);
}
#endif

FLRGuardAwarenessSnapshot ALRGuardAIController::BuildCurrentAwarenessSnapshot() const
{
	FLRGuardAwarenessSnapshot snapshot;
	if (Alert.IsValid())
	{
		snapshot.Alert = Alert->GetAlertSnapshot();
	}
	if (Knowledge.IsValid())
	{
		snapshot.Knowledge = Knowledge->GetSnapshot();
		snapshot.InvestigationLocation = LRAlertRules::ResolveInvestigationLocation(snapshot.Knowledge);
	}
	snapshot.ResolvedBehavior = LRAlertRules::ResolveTargetBehavior(bStunned,
		snapshot.Alert, snapshot.Knowledge);
	snapshot.Alert.Behavior = snapshot.ResolvedBehavior;
	return snapshot;
}

FLRGuardAwarenessSnapshot ALRGuardAIController::GetAwarenessSnapshot() const
{
	return CachedAwareness;
}

ELRGuardBehaviorState ALRGuardAIController::GetResolvedBehavior() const
{
	return BuildCurrentAwarenessSnapshot().ResolvedBehavior;
}

void ALRGuardAIController::ReceiveNoiseStimulus(const FLRGuardNoiseStimulus& stimulus)
{
	HandleAttractStimulus(stimulus);
}

void ALRGuardAIController::MarkInvestigationReached()
{
	if (!Alert.IsValid() || !Knowledge.IsValid())
	{
		return;
	}
	SetInvestigationAtLocation();
	ProcessAwarenessTransaction(LRGameplayTags::InvestigationReached, true);
}

void ALRGuardAIController::MarkInvestigationUnreachable()
{
	if (!Alert.IsValid() || !Knowledge.IsValid())
	{
		return;
	}
	SetInvestigationFailed();
	ProcessAwarenessTransaction(LRGameplayTags::InvestigationUnreachable, true);
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

void ALRGuardAIController::HandleAlertDecayRequested()
{
	if (!Alert.IsValid() || !Knowledge.IsValid() || Alert->GetAlertLevel() <= 0)
	{
		return;
	}

	const FLRGuardAwarenessSnapshot previous = BuildCurrentAwarenessSnapshot();
	Alert->ApplyDelta(-GetEffectiveTuning().AlertDecayAmount);
	if (Alert->GetAlertLevel() == LRAlertRules::MinAlertLevel)
	{
		Knowledge->ResetAwareness();
		ClearInvestigationMoveRequest();
		bSightToChaseGraceConsumed = false;
	}
	ProcessAwarenessTransaction(LRGameplayTags::AlertDecay);
	(void)previous;
}

void ALRGuardAIController::HandleStunEnd()
{
	bStunned = false;
	ProcessAwarenessTransaction(LRGameplayTags::TargetGuardCourageVulnerable, true);
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
	const float duration = StateTuning ? StateTuning->CourageKnockbackDurationSeconds : 0.6f;
	UE_LOG(LogLostRunicAI, Display, TEXT("Guard=%s stunned for %.2fs direction=%s"),
		*GetNameSafe(GetPawn()), duration, *direction.ToCompactString());
	ProcessAwarenessTransaction(LRGameplayTags::TargetGuardCourageVulnerable, true);
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(StunTimer, this, &ALRGuardAIController::HandleStunEnd,
			duration, false);
	}
}

const FLRGuardTuningSettings& ALRGuardAIController::GetEffectiveTuning() const
{
	return Tuning;
}
