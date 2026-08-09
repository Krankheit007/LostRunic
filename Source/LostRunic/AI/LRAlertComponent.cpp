#include "AI/LRAlertComponent.h"

#include "AI/LRAlertRules.h"
#include "Core/LRGameplayTags.h"
#include "Core/LRLog.h"
#include "Data/LRGameTuningSet.h"
#include "Data/LRGuardTuning.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "TimerManager.h"

ULRAlertComponent::ULRAlertComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULRAlertComponent::BeginPlay()
{
	Super::BeginPlay();
	const UGameInstance* gameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const ULRGameInstanceSubsystem* subsystem = gameInstance ? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	Tuning = subsystem && subsystem->GetTuningSet() ? subsystem->GetTuningSet()->Guard : nullptr;
	if (!ensureMsgf(Tuning, TEXT("%s requires Guard tuning."), *GetNameSafe(this)))
	{
		return;
	}
	LastStimulusTimeSeconds = GetWorld()->GetTimeSeconds();
	GetWorld()->GetTimerManager().SetTimer(DecayTimer, this, &ULRAlertComponent::HandleDecayTimer,
		Tuning->AlertDecayIntervalSeconds, true);
}

void ULRAlertComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(DecayTimer);
	}
	Super::EndPlay(endPlayReason);
}

void ULRAlertComponent::ApplyAlertDelta(const int32 delta, const FVector location, AActor* target,
	const FGameplayTag reason)
{
	const int32 previousLevel = AlertLevel;
	const ELRGuardBehaviorState previousState = GetBehaviorState();
	AlertLevel = LRAlertRules::ApplyDelta(AlertLevel, delta);
	LastDisturbanceLocation = location;
	if (target)
	{
		TargetActor = target;
	}
	if (delta > 0)
	{
		bSearching = false;
	}
	LastStimulusTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	BroadcastChange(previousLevel, previousState, reason);
}

void ULRAlertComponent::SetSightTarget(AActor* target, const bool bVisible, const FVector lastKnownLocation)
{
	const int32 previousLevel = AlertLevel;
	const ELRGuardBehaviorState previousState = GetBehaviorState();
	LastDisturbanceLocation = lastKnownLocation;
	TargetActor = target;
	bHasConfirmedSight = bVisible;
	bSearching = !bVisible && AlertLevel > 0;
	if (bVisible)
	{
		AlertLevel = FMath::Max(AlertLevel, GetEffectiveTuning().SightAlertLevel);
	}
	LastStimulusTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	BroadcastChange(previousLevel, previousState,
		bVisible ? LRGameplayTags::SightPlayer : LRGameplayTags::SightPlayerLost);
}

void ULRAlertComponent::MarkInvestigationReached()
{
	const ELRGuardBehaviorState previousState = GetBehaviorState();
	bSearching = AlertLevel > 0;
	BroadcastChange(AlertLevel, previousState, LRGameplayTags::SearchReached);
}

void ULRAlertComponent::ResetAfterSearch()
{
	const int32 previousLevel = AlertLevel;
	const ELRGuardBehaviorState previousState = GetBehaviorState();
	AlertLevel = 0;
	bSearching = false;
	bHasConfirmedSight = false;
	TargetActor.Reset();
	BroadcastChange(previousLevel, previousState, LRGameplayTags::SearchTimeout);
}

ELRGuardBehaviorState ULRAlertComponent::GetBehaviorState() const
{
	return LRAlertRules::ResolveState(AlertLevel, bHasConfirmedSight, bSearching);
}

void ULRAlertComponent::HandleDecayTimer()
{
	if (AlertLevel <= 0 || !GetWorld())
	{
		return;
	}
	const float elapsed = GetWorld()->GetTimeSeconds() - LastStimulusTimeSeconds;
	if (LRAlertRules::ShouldDecay(elapsed, GetEffectiveTuning().InitialObserveSeconds, bHasConfirmedSight))
	{
		ApplyAlertDelta(-GetEffectiveTuning().AlertDecayAmount, LastDisturbanceLocation,
			TargetActor.Get(), LRGameplayTags::SearchAlertDecay);
	}
}

void ULRAlertComponent::BroadcastChange(const int32 previousLevel, const ELRGuardBehaviorState previousState,
	const FGameplayTag reason)
{
	LastReason = reason;
	const ELRGuardBehaviorState currentState = GetBehaviorState();
	UE_LOG(LogLostRunicAI, Display, TEXT("Guard=%s alert %d -> %d state %d -> %d reason=%s location=%s"),
		*GetNameSafe(GetOwner()), previousLevel, AlertLevel, static_cast<int32>(previousState),
		static_cast<int32>(currentState), *reason.ToString(), *LastDisturbanceLocation.ToCompactString());
	OnAlertChanged.Broadcast(previousLevel, AlertLevel, currentState, reason, LastDisturbanceLocation);
}

const ULRGuardTuning& ULRAlertComponent::GetEffectiveTuning() const
{
	return Tuning ? *Tuning : *GetDefault<ULRGuardTuning>();
}
