#include "State/LRStateComponent.h"

#include "Core/LRGameplayTags.h"
#include "Core/LRLog.h"
#include "Data/LRGameTuningSet.h"
#include "Data/LRStateTuning.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "TimerManager.h"

ULRStateComponent::ULRStateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULRStateComponent::BeginPlay()
{
	Super::BeginPlay();
	const UGameInstance* gameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const ULRGameInstanceSubsystem* subsystem = gameInstance ? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	Tuning = subsystem && subsystem->GetTuningSet() ? subsystem->GetTuningSet()->State : nullptr;
	ensureMsgf(Tuning, TEXT("%s is using fallback State tuning because the project tuning set is unavailable."), *GetNameSafe(this));
}

void ULRStateComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
	}
	Super::EndPlay(endPlayReason);
}

FLRStateChangeResult ULRStateComponent::RequestStateChange(const FLRStateChangeRequest& request)
{
	FLRStateChangeResult result;
	result.PreviousMode = CurrentMode;
	result.CurrentMode = CurrentMode;
	const bool bDeathRequest = request.RequestType == ELRStateRequestType::Death;

	FGameplayTag rejectionReason;
	if (request.TargetMode == CurrentMode)
	{
		rejectionReason = LRGameplayTags::StateRejectAlreadyCurrent;
	}
	else if (bPresentationLocked && !bDeathRequest)
	{
		rejectionReason = LRGameplayTags::StateRejectPresentationLocked;
	}
	else if (!ActiveBlockers.IsEmpty() && !bDeathRequest)
	{
		rejectionReason = LRGameplayTags::StateRejectBlocked;
	}
	else if (!LRStateRules::IsTransitionAllowed(CurrentMode, request)
		|| request.Source != LRStateRules::GetSourceTag(request.RequestType))
	{
		rejectionReason = LRGameplayTags::StateRejectInvalidTransition;
	}

	if (rejectionReason.IsValid())
	{
		result.Reason = rejectionReason;
		RejectRequest(request, rejectionReason);
		return result;
	}

	if (bDeathRequest && bPresentationLocked)
	{
		NotifyPresentationComplete();
	}

	const ELRPerceptionMode previousMode = CurrentMode;
	StartPresentationLock();
	OnStateChanging.Broadcast(previousMode, request.TargetMode, request.Source);
	CurrentMode = request.TargetMode;
	LastTransitionReason = request.Source;
	OnStateChanged.Broadcast(CurrentMode, request.Source);

	result.bAccepted = true;
	result.CurrentMode = CurrentMode;
	result.Reason = request.Source;
	UE_LOG(LogLostRunicState, Log, TEXT("Owner=%s state %d -> %d source=%s"), *GetNameSafe(GetOwner()),
		static_cast<int32>(previousMode), static_cast<int32>(CurrentMode), *request.Source.ToString());
	return result;
}

void ULRStateComponent::BeginEyeInput(const ELRStateRequestType inputType)
{
	if (!InputGate.Press(inputType))
	{
		FLRStateChangeRequest request;
		request.RequestType = inputType;
		request.Source = LRStateRules::GetSourceTag(inputType);
		request.TargetMode = CurrentMode;
		RejectRequest(request, LRGameplayTags::StateRejectConcurrentInput);
		return;
	}

	float holdSeconds = 0.0f;
	if (!LRStateRules::ResolveEyeTransition(CurrentMode, inputType, GetEffectiveTuning(), PendingInputTarget, holdSeconds)
		|| bPresentationLocked || !ActiveBlockers.IsEmpty())
	{
		FLRStateChangeRequest request;
		request.RequestType = inputType;
		request.Source = LRStateRules::GetSourceTag(inputType);
		request.TargetMode = PendingInputTarget;
		RejectRequest(request, bPresentationLocked ? LRGameplayTags::StateRejectPresentationLocked
			: !ActiveBlockers.IsEmpty() ? LRGameplayTags::StateRejectBlocked : LRGameplayTags::StateRejectInvalidTransition);
		InputGate.ConsumeThreshold(inputType);
		return;
	}

	OnHoldStarted.Broadcast(inputType, PendingInputTarget, holdSeconds);
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(HoldTimer, this, &ULRStateComponent::HandleHoldThreshold, holdSeconds, false);
	}
}

void ULRStateComponent::EndEyeInput(const ELRStateRequestType inputType)
{
	if (InputGate.GetActiveInput() == inputType && GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(HoldTimer);
	}
	if (InputGate.Release(inputType))
	{
		OnHoldCanceled.Broadcast(inputType);
	}
}

void ULRStateComponent::CancelEyeInputSequence()
{
	const ELRStateRequestType activeInput = InputGate.GetActiveInput();
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(HoldTimer);
	}
	InputGate.Reset();
	if (activeInput != ELRStateRequestType::None)
	{
		OnHoldCanceled.Broadcast(activeInput);
	}
}

void ULRStateComponent::SetBlockerActive(const FGameplayTag blocker, const bool bActive)
{
	if (!blocker.IsValid())
	{
		return;
	}
	if (bActive)
	{
		ActiveBlockers.AddTag(blocker);
		CancelEyeInputSequence();
	}
	else
	{
		ActiveBlockers.RemoveTag(blocker);
	}
}

void ULRStateComponent::NotifyPresentationComplete()
{
	if (!bPresentationLocked)
	{
		return;
	}
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(PresentationSafetyTimer);
	}
	bPresentationLocked = false;
	OnPresentationLockChanged.Broadcast(false);
}

void ULRStateComponent::LogDiagnostics() const
{
	UE_LOG(LogLostRunicState, Display, TEXT("Owner=%s Mode=%d PresentationLocked=%s LastReason=%s Blockers=%s"),
		*GetNameSafe(GetOwner()), static_cast<int32>(CurrentMode), bPresentationLocked ? TEXT("true") : TEXT("false"),
		*LastTransitionReason.ToString(), *ActiveBlockers.ToStringSimple());
}

void ULRStateComponent::HandleHoldThreshold()
{
	const ELRStateRequestType inputType = InputGate.GetActiveInput();
	if (!InputGate.ConsumeThreshold(inputType))
	{
		return;
	}
	OnHoldThresholdReached.Broadcast(inputType, PendingInputTarget);

	FLRStateChangeRequest request;
	request.TargetMode = PendingInputTarget;
	request.RequestType = inputType;
	request.Source = LRStateRules::GetSourceTag(inputType);
	RequestStateChange(request);
}

void ULRStateComponent::HandlePresentationSafetyTimeout()
{
	UE_LOG(LogLostRunicState, Warning, TEXT("Owner=%s presentation lock reached its %.2fs safety timeout."),
		*GetNameSafe(GetOwner()), GetEffectiveTuning().PresentationSafetyTimeoutSeconds);
	NotifyPresentationComplete();
}

void ULRStateComponent::RejectRequest(const FLRStateChangeRequest& request, const FGameplayTag reason)
{
	OnStateChangeRejected.Broadcast(request, reason);
	UE_LOG(LogLostRunicState, Verbose, TEXT("Owner=%s rejected target=%d request=%d reason=%s"), *GetNameSafe(GetOwner()),
		static_cast<int32>(request.TargetMode), static_cast<int32>(request.RequestType), *reason.ToString());
}

void ULRStateComponent::StartPresentationLock()
{
	bPresentationLocked = true;
	OnPresentationLockChanged.Broadcast(true);
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(PresentationSafetyTimer, this,
			&ULRStateComponent::HandlePresentationSafetyTimeout, GetEffectiveTuning().PresentationSafetyTimeoutSeconds, false);
	}
}

const ULRStateTuning& ULRStateComponent::GetEffectiveTuning() const
{
	return Tuning ? *Tuning : *GetDefault<ULRStateTuning>();
}
