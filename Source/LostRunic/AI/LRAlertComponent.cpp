/**
 * @file LRAlertComponent.cpp
 * @brief Implements the pure alert meter and its local observation/decay clocks.
 */
#include "AI/LRAlertComponent.h"

#include "AI/LRAlertRules.h"
#include "Core/LRLog.h"
#include "Engine/World.h"
#include "TimerManager.h"

ULRAlertComponent::ULRAlertComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULRAlertComponent::BeginPlay()
{
	Super::BeginPlay();
}

void ULRAlertComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	ShutdownRuntime();
	OnDecayRequested.Clear();
	Super::EndPlay(endPlayReason);
}

void ULRAlertComponent::InitializeRuntime(const FLRGuardTuningSettings& tuning)
{
	RuntimeTuning = tuning;
	bRuntimeInitialized = true;
	if (!GetWorld())
	{
		return;
	}

	FTimerManager& timers = GetWorld()->GetTimerManager();
	timers.SetTimer(DecayTimer, this, &ULRAlertComponent::HandleDecayTimer,
		RuntimeTuning.AlertDecayIntervalSeconds, true);
	if (bObserving)
	{
		const float remainingSeconds = static_cast<float>(ObservationEndTimeSeconds - GetWorld()->GetTimeSeconds());
		if (remainingSeconds > 0.0f)
		{
			timers.SetTimer(ObservationTimer, this, &ULRAlertComponent::HandleObservationEnd,
				remainingSeconds, false);
		}
		else
		{
			HandleObservationEnd();
		}
	}
}

void ULRAlertComponent::ShutdownRuntime()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(DecayTimer);
		GetWorld()->GetTimerManager().ClearTimer(ObservationTimer);
	}
	RuntimeTuning = FLRGuardTuningSettings();
	bRuntimeInitialized = false;
}

bool ULRAlertComponent::ApplyDelta(const int32 delta)
{
	const int32 previousLevel = AlertLevel;
	AlertLevel = LRAlertRules::ApplyDelta(AlertLevel, delta);
	if (delta > 0)
	{
		bSearching = false;
	}
	if (AlertLevel <= 0)
	{
		ClearWhenAlertZero();
	}
	return previousLevel != AlertLevel;
}

bool ULRAlertComponent::RaiseToMinimum(const int32 minimumLevel)
{
	return ApplyDelta(FMath::Max(minimumLevel - AlertLevel, 0));
}

bool ULRAlertComponent::LowerToMaximum(const int32 maximumLevel)
{
	return ApplyDelta(FMath::Min(maximumLevel - AlertLevel, 0));
}

bool ULRAlertComponent::TryApplyAttract(const double nowSeconds)
{
	const FLRGuardTuningSettings& tuning = GetEffectiveTuning();
	if (AlertLevel > 0 && !LRAlertRules::IsIncreaseAllowed(nowSeconds, LastIncreaseTimeSeconds,
		LRAlertRules::ResolveAttractIncreaseCooldown(AlertLevel, bFirstIncreaseInBand, tuning)))
	{
		return false;
	}

	const bool bCrossingIntoBand = AlertLevel < tuning.DetectionInvestigateAlertFloor;
	ApplyDelta(tuning.AttractAlertAmount);
	LastIncreaseTimeSeconds = nowSeconds;
	if (bFirstIncreaseInBand)
	{
		bFirstIncreaseInBand = false;
	}
	if (bCrossingIntoBand && AlertLevel >= tuning.DetectionInvestigateAlertFloor)
	{
		bFirstIncreaseInBand = true;
	}
	StartObservation();
	return true;
}

void ULRAlertComponent::MarkInvestigationReached()
{
	bSearching = AlertLevel > 0;
	StartObservation();
}

void ULRAlertComponent::MarkInvestigationUnreachable()
{
	bSearching = AlertLevel > 0;
	StartObservation();
}

void ULRAlertComponent::ResetAfterSearch()
{
	AlertLevel = 0;
	ClearWhenAlertZero();
}

void ULRAlertComponent::HandleDecayTimer()
{
	if (AlertLevel > 0)
	{
		OnDecayRequested.Broadcast();
	}
}

void ULRAlertComponent::HandleObservationEnd()
{
	bObserving = false;
	ObservationEndTimeSeconds = 0.0;
}

void ULRAlertComponent::StartObservation()
{
	if (AlertLevel <= 0 || !GetWorld() || !bRuntimeInitialized)
	{
		return;
	}
	bObserving = true;
	ObservationEndTimeSeconds = GetWorld()->GetTimeSeconds() + GetEffectiveTuning().InitialObserveSeconds;
	GetWorld()->GetTimerManager().ClearTimer(ObservationTimer);
	GetWorld()->GetTimerManager().SetTimer(ObservationTimer, this, &ULRAlertComponent::HandleObservationEnd,
		GetEffectiveTuning().InitialObserveSeconds, false);
}

void ULRAlertComponent::ClearWhenAlertZero()
{
	bSearching = false;
	bObserving = false;
	ObservationEndTimeSeconds = 0.0;
	bFirstIncreaseInBand = false;
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(ObservationTimer);
	}
}

FLRAlertSnapshot ULRAlertComponent::GetAlertSnapshot() const
{
	FLRAlertSnapshot snapshot;
	snapshot.Level = AlertLevel;
	snapshot.Fraction = AlertLevel / static_cast<float>(LRAlertRules::MaxAlertLevel);
	snapshot.Tier = LRAlertRules::ResolveAlertTier(AlertLevel);
	snapshot.bFullAlert = AlertLevel >= LRAlertRules::MaxAlertLevel;
	return snapshot;
}

void ULRAlertComponent::PublishCommittedChange(const int32 previousLevel,
	const ELRGuardBehaviorState resolvedBehavior, const FGameplayTag reason, const FVector& location)
{
	LastReason = reason;
	FLRAlertSnapshot snapshot = GetAlertSnapshot();
	snapshot.Behavior = resolvedBehavior;
	UE_LOG(LogLostRunicAI, Display, TEXT("Guard=%s alert %d -> %d behavior=%d reason=%s location=%s"),
		*GetNameSafe(GetOwner()), previousLevel, AlertLevel, static_cast<int32>(resolvedBehavior),
		*reason.GetTagName().ToString(), *location.ToCompactString());
	OnAlertChanged.Broadcast(previousLevel, AlertLevel, resolvedBehavior, reason, location);
	OnAlertSnapshotChanged.Broadcast(snapshot);
}

const FLRGuardTuningSettings& ULRAlertComponent::GetEffectiveTuning() const
{
	return RuntimeTuning;
}
