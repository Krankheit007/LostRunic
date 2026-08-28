/**
 * @file LRAlertComponent.cpp
 * @brief 实现 Guard 警戒值、白/红观察、自然衰减和噪声冷却。
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
	timers.ClearTimer(DecayTimer);
	timers.ClearTimer(ObservationTimer);
	timers.ClearTimer(AttractCooldownTimer);

	const double nowSeconds = GetWorld()->GetTimeSeconds();
	if (TimerMode == ELRGuardAlertTimerMode::Decay && AlertLevel > 0)
	{
		StartDecay();
	}
	else if ((TimerMode == ELRGuardAlertTimerMode::WhiteObservation
		|| TimerMode == ELRGuardAlertTimerMode::RedObservation) && AlertLevel > 0)
	{
		const float remainingSeconds = static_cast<float>(ObservationEndTimeSeconds - nowSeconds);
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
	if (AttractCooldownEndTimeSeconds > nowSeconds)
	{
		timers.SetTimer(AttractCooldownTimer, this, &ULRAlertComponent::HandleAttractCooldownEnd,
			static_cast<float>(AttractCooldownEndTimeSeconds - nowSeconds), false);
	}
}

void ULRAlertComponent::ShutdownRuntime()
{
	if (GetWorld())
	{
		FTimerManager& timers = GetWorld()->GetTimerManager();
		timers.ClearTimer(DecayTimer);
		timers.ClearTimer(ObservationTimer);
		timers.ClearTimer(AttractCooldownTimer);
	}
	bRuntimeInitialized = false;
	RuntimeTuning = FLRGuardTuningSettings();
}

bool ULRAlertComponent::ApplyDelta(const int32 delta)
{
	const int32 previousLevel = AlertLevel;
	AlertLevel = LRAlertRules::ApplyDelta(AlertLevel, delta);
	if (previousLevel >= LRAlertRules::InvestigateMinLevel
		&& AlertLevel <= LRAlertRules::SuspiciousMaxLevel)
	{
		bRedAttractAccepted = false;
		TimerMode = TimerMode == ELRGuardAlertTimerMode::Decay
			? ELRGuardAlertTimerMode::Decay : ELRGuardAlertTimerMode::None;
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().ClearTimer(ObservationTimer);
		}
		ObservationEndTimeSeconds = 0.0;
	}
	if (AlertLevel <= LRAlertRules::MinAlertLevel)
	{
		ResetToZero();
	}
	return previousLevel != AlertLevel;
}

void ULRAlertComponent::ApplySightAlertLevel()
{
	const int32 previousLevel = AlertLevel;
	AlertLevel = LRAlertRules::InvestigateMinLevel;
	StopObservationAndDecay();
	if (previousLevel != AlertLevel)
	{
		UE_LOG(LogLostRunicAI, Verbose, TEXT("Guard=%s sight entered red alert level=%d"),
			*GetNameSafe(GetOwner()), AlertLevel);
	}
}

bool ULRAlertComponent::CanAcceptAttract(const double nowSeconds) const
{
	return nowSeconds >= AttractCooldownEndTimeSeconds;
}

bool ULRAlertComponent::IsAttractCooldownActive() const
{
	if (AttractCooldownEndTimeSeconds <= 0.0)
	{
		return false;
	}
	const double nowSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	return nowSeconds < AttractCooldownEndTimeSeconds;
}

bool ULRAlertComponent::IsFirstAttractInResultBand(const int32 resultAlertLevel) const
{
	if (resultAlertLevel >= LRAlertRules::InvestigateMinLevel)
	{
		return !bRedAttractAccepted;
	}
	if (resultAlertLevel >= LRAlertRules::SuspiciousMinLevel)
	{
		return !bWhiteAttractAccepted;
	}
	return false;
}

bool ULRAlertComponent::ApplyAcceptedAttract(const int32 resultAlertLevel, const double nowSeconds,
	const float cooldownSeconds, const bool bStartWhiteObservation)
{
	const int32 previousLevel = AlertLevel;
	const bool bResultInWhite = resultAlertLevel >= LRAlertRules::SuspiciousMinLevel
		&& resultAlertLevel <= LRAlertRules::SuspiciousMaxLevel;
	AlertLevel = FMath::Clamp(resultAlertLevel, LRAlertRules::MinAlertLevel,
		LRAlertRules::InvestigateMaxLevel);

	if (bResultInWhite)
	{
		bWhiteAttractAccepted = true;
		StopObservationAndDecay();
		if (bStartWhiteObservation)
		{
			StartWhiteObservation();
		}
	}
	else if (AlertLevel >= LRAlertRules::InvestigateMinLevel)
	{
		bRedAttractAccepted = true;
		StopObservationAndDecay();
	}

	StartAttractCooldown(nowSeconds, cooldownSeconds);
	return previousLevel != AlertLevel || bResultInWhite;
}

void ULRAlertComponent::StartWhiteObservation()
{
	StartObservation(ELRGuardAlertTimerMode::WhiteObservation);
}

void ULRAlertComponent::StartRedObservation()
{
	if (AlertLevel >= LRAlertRules::InvestigateMinLevel
		&& AlertLevel <= LRAlertRules::InvestigateMaxLevel)
	{
		StartObservation(ELRGuardAlertTimerMode::RedObservation);
	}
}

void ULRAlertComponent::StopObservationAndDecay()
{
	TimerMode = ELRGuardAlertTimerMode::None;
	ObservationEndTimeSeconds = 0.0;
	if (GetWorld())
	{
		FTimerManager& timers = GetWorld()->GetTimerManager();
		timers.ClearTimer(ObservationTimer);
		timers.ClearTimer(DecayTimer);
	}
}

void ULRAlertComponent::ResetToZero()
{
	AlertLevel = 0;
	bWhiteAttractAccepted = false;
	bRedAttractAccepted = false;
	AttractCooldownEndTimeSeconds = 0.0;
	TimerMode = ELRGuardAlertTimerMode::None;
	ObservationEndTimeSeconds = 0.0;
	if (GetWorld())
	{
		FTimerManager& timers = GetWorld()->GetTimerManager();
		timers.ClearTimer(DecayTimer);
		timers.ClearTimer(ObservationTimer);
		timers.ClearTimer(AttractCooldownTimer);
	}
}

void ULRAlertComponent::HandleDecayTimer()
{
	if (TimerMode == ELRGuardAlertTimerMode::Decay && AlertLevel > 0)
	{
		OnDecayRequested.Broadcast();
	}
}

void ULRAlertComponent::HandleObservationEnd()
{
	if (TimerMode != ELRGuardAlertTimerMode::WhiteObservation
		&& TimerMode != ELRGuardAlertTimerMode::RedObservation)
	{
		return;
	}
	ObservationEndTimeSeconds = 0.0;
	StartDecay();
}

void ULRAlertComponent::HandleAttractCooldownEnd()
{
	AttractCooldownEndTimeSeconds = 0.0;
}

void ULRAlertComponent::StartObservation(const ELRGuardAlertTimerMode mode)
{
	if (AlertLevel <= 0)
	{
		return;
	}

	TimerMode = mode;
	ObservationEndTimeSeconds = GetWorld()
		? GetWorld()->GetTimeSeconds() + (mode == ELRGuardAlertTimerMode::WhiteObservation
			? RuntimeTuning.SuspiciousObserveSeconds : RuntimeTuning.InvestigateObserveSeconds)
		: 0.0;
	if (!GetWorld() || !bRuntimeInitialized)
	{
		return;
	}

	FTimerManager& timers = GetWorld()->GetTimerManager();
	timers.ClearTimer(DecayTimer);
	timers.ClearTimer(ObservationTimer);
	const float duration = mode == ELRGuardAlertTimerMode::WhiteObservation
		? RuntimeTuning.SuspiciousObserveSeconds : RuntimeTuning.InvestigateObserveSeconds;
	timers.SetTimer(ObservationTimer, this, &ULRAlertComponent::HandleObservationEnd, duration, false);
}

void ULRAlertComponent::StartDecay()
{
	if (AlertLevel <= 0)
	{
		ResetToZero();
		return;
	}

	TimerMode = ELRGuardAlertTimerMode::Decay;
	if (GetWorld() && bRuntimeInitialized)
	{
		FTimerManager& timers = GetWorld()->GetTimerManager();
		timers.ClearTimer(ObservationTimer);
		timers.ClearTimer(DecayTimer);
		timers.SetTimer(DecayTimer, this, &ULRAlertComponent::HandleDecayTimer,
			RuntimeTuning.AlertDecayIntervalSeconds, true);
	}
}

void ULRAlertComponent::StartAttractCooldown(const double nowSeconds, const float cooldownSeconds)
{
	AttractCooldownEndTimeSeconds = cooldownSeconds > 0.0f ? nowSeconds + cooldownSeconds : 0.0;
	if (GetWorld() && bRuntimeInitialized)
	{
		FTimerManager& timers = GetWorld()->GetTimerManager();
		timers.ClearTimer(AttractCooldownTimer);
		if (AttractCooldownEndTimeSeconds > nowSeconds)
		{
			timers.SetTimer(AttractCooldownTimer, this, &ULRAlertComponent::HandleAttractCooldownEnd,
				cooldownSeconds, false);
		}
	}
}

void ULRAlertComponent::ClearWhenAlertZero()
{
	ResetToZero();
}

FLRAlertSnapshot ULRAlertComponent::GetAlertSnapshot() const
{
	FLRAlertSnapshot snapshot;
	snapshot.Level = FMath::Clamp(AlertLevel, LRAlertRules::MinAlertLevel, LRAlertRules::MaxAlertLevel);
	if (snapshot.Level >= LRAlertRules::ConfirmedAlertLevel)
	{
		snapshot.Fraction = 1.0f;
	}
	else if (snapshot.Level >= LRAlertRules::InvestigateMinLevel)
	{
		snapshot.Fraction = (snapshot.Level - LRAlertRules::SuspiciousMaxLevel)
			/ static_cast<float>(LRAlertRules::SuspiciousMaxLevel);
	}
	else if (snapshot.Level >= LRAlertRules::SuspiciousMinLevel)
	{
		snapshot.Fraction = snapshot.Level / static_cast<float>(LRAlertRules::SuspiciousMaxLevel);
	}
	snapshot.Tier = LRAlertRules::ResolveAlertTier(snapshot.Level);
	snapshot.bFullAlert = snapshot.Level == LRAlertRules::ConfirmedAlertLevel;
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
