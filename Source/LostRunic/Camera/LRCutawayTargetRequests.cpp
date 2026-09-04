#include "Camera/LRCutawayTargetComponent.h"

#include "Core/LRCustomPrimitiveData.h"
#include "Engine/World.h"
#include "TimerManager.h"

namespace
{
	// Weak requester cleanup is intentionally slower than the visual transition tick.
	constexpr float RequesterAuditIntervalSeconds = 0.25f;
}

bool ULRCutawayTargetComponent::SetCutawayRequest(UObject* requester, const ELRCutawayRequestType requestType,
	const float amount, const bool bImmediate)
{
	if (!IsValid(requester) || !SupportsRequest(requestType) || !FMath::IsFinite(amount)) return false;
	FRequestMap& requests = GetRequestMap(requestType);
	const float clampedAmount = FMath::Clamp(amount, 0.0f, 1.0f);
	if (clampedAmount <= UE_KINDA_SMALL_NUMBER) requests.Remove(requester);
	else requests.FindOrAdd(requester) = clampedAmount;
	UpdateChannelTarget(requestType, bImmediate);
	RefreshRequesterAuditTimer();
	return true;
}

void ULRCutawayTargetComponent::ClearCutawayRequest(UObject* requester, const ELRCutawayRequestType requestType,
	const bool bImmediate)
{
	if (!requester) return;
	GetRequestMap(requestType).Remove(requester);
	UpdateChannelTarget(requestType, bImmediate);
	RefreshRequesterAuditTimer();
}

void ULRCutawayTargetComponent::SetForegroundCutawayEnabled(const bool bEnabled, const bool bImmediate)
{
	bEnableForegroundCutaway = bEnabled;
	if (bEnabled) SetCutawayRequest(this, ELRCutawayRequestType::Foreground, 1.0f, bImmediate);
	else ClearCutawayRequest(this, ELRCutawayRequestType::Foreground, bImmediate);
}

bool ULRCutawayTargetComponent::SupportsRequest(const ELRCutawayRequestType requestType) const
{
	switch (requestType)
	{
	case ELRCutawayRequestType::Local: return bEnableLocalCutaway;
	case ELRCutawayRequestType::Group: return bEnableGroupCutaway;
	case ELRCutawayRequestType::Foreground: return bEnableForegroundCutaway;
	default: return false;
	}
}

void ULRCutawayTargetComponent::AuditRequesterLifetimes()
{
	const ELRCutawayRequestType requestTypes[] = {
		ELRCutawayRequestType::Local, ELRCutawayRequestType::Group, ELRCutawayRequestType::Foreground
	};
	for (const ELRCutawayRequestType requestType : requestTypes)
	{
		FRequestMap& requests = GetRequestMap(requestType);
		const float requestedAmount = GetRequestedAmount(requests);
		if (!FMath::IsNearlyEqual(requestedAmount, GetChannel(requestType).TargetAmount))
		{
			UpdateChannelTarget(requestType, false);
		}
	}
	RefreshRequesterAuditTimer();
}

void ULRCutawayTargetComponent::RefreshRequesterAuditTimer()
{
	if (!GetWorld()) return;
	FTimerManager& timerManager = GetWorld()->GetTimerManager();
	if (HasExternalRequests())
	{
		if (!timerManager.IsTimerActive(RequesterAuditTimer))
		{
			timerManager.SetTimer(RequesterAuditTimer, this, &ThisClass::AuditRequesterLifetimes,
				RequesterAuditIntervalSeconds, true);
		}
	}
	else timerManager.ClearTimer(RequesterAuditTimer);
}

bool ULRCutawayTargetComponent::HasExternalRequests() const
{
	const FRequestMap* requestMaps[] = { &LocalRequests, &GroupRequests, &ForegroundRequests };
	for (const FRequestMap* requests : requestMaps)
	{
		for (const TPair<TWeakObjectPtr<UObject>, float>& request : *requests)
		{
			if (request.Key.Get() != this) return true;
		}
	}
	return false;
}

void ULRCutawayTargetComponent::UpdateChannelTarget(const ELRCutawayRequestType requestType,
	const bool bImmediate)
{
	FRequestMap& requests = GetRequestMap(requestType);
	const float target = GetRequestedAmount(requests);
	FLRCutawayChannelState& channel = GetMutableChannel(requestType);
	channel.Retarget(target, GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0, HideDuration, RestoreDuration, bImmediate);
	WriteAmount(requestType, channel.CurrentAmount);
	SetComponentTickEnabled(channel.IsTransitioning() || LocalState.IsTransitioning()
		|| GroupState.IsTransitioning() || ForegroundState.IsTransitioning());
}

void ULRCutawayTargetComponent::WriteAmount(const ELRCutawayRequestType requestType, const float amount)
{
	int32 index = LRCustomPrimitiveData::LocalAmount;
	if (requestType == ELRCutawayRequestType::Group) index = LRCustomPrimitiveData::GroupAmount;
	if (requestType == ELRCutawayRequestType::Foreground) index = LRCustomPrimitiveData::ForegroundAmount;
	WritePrimitiveDataFloat(index, amount);
}

ULRCutawayTargetComponent::FRequestMap& ULRCutawayTargetComponent::GetRequestMap(
	const ELRCutawayRequestType requestType)
{
	if (requestType == ELRCutawayRequestType::Group) return GroupRequests;
	if (requestType == ELRCutawayRequestType::Foreground) return ForegroundRequests;
	return LocalRequests;
}

const FLRCutawayChannelState& ULRCutawayTargetComponent::GetChannel(const ELRCutawayRequestType requestType) const
{
	if (requestType == ELRCutawayRequestType::Group) return GroupState;
	if (requestType == ELRCutawayRequestType::Foreground) return ForegroundState;
	return LocalState;
}

FLRCutawayChannelState& ULRCutawayTargetComponent::GetMutableChannel(const ELRCutawayRequestType requestType)
{
	if (requestType == ELRCutawayRequestType::Group) return GroupState;
	if (requestType == ELRCutawayRequestType::Foreground) return ForegroundState;
	return LocalState;
}

float ULRCutawayTargetComponent::GetRequestedAmount(FRequestMap& requests)
{
	float result = 0.0f;
	for (auto iterator = requests.CreateIterator(); iterator; ++iterator)
	{
		if (!iterator.Key().IsValid()) iterator.RemoveCurrent();
		else result = FMath::Max(result, iterator.Value());
	}
	return result;
}
