/**
 * @file LRPerceptionPresentationComponent_Echo.cpp
 * @brief Applies visual pulse events to fixed Echo slots and optional Niagara decoration.
 */
#include "Perception/LRPerceptionPresentationComponent.h"

#include "Data/LRPresentationTuning.h"
#include "Data/LRVisualStyleDefinition.h"
#include "Engine/World.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Perception/LRPerceptionEventSubsystem.h"
#include "Perception/LRPerceptionRules.h"
#include "Perception/LRPerceptionSoundSourceComponent.h"

namespace
{
	float ResolveVisualRadius(const FLRPerceptionPulseRequest& request,
		const ULRPresentationTuning* tuning)
	{
		const float configuredRadius = request.VisualRadiusCm > 0.0f
			? request.VisualRadiusCm : (tuning ? tuning->NoiseRevealRadius : 200.0f);
		return FMath::Max(configuredRadius, 1.0f);
	}
}

void ULRPerceptionPresentationComponent::ApplyPulse(const FLRPerceptionPulseRequest& request)
{
	if (!bPerceptionActive || !GetWorld())
	{
		return;
	}
	const float now = GetCurrentGameTime();
	const int32 refreshIndex = FindRefreshSlot(request, now);
	const int32 slotIndex = refreshIndex != INDEX_NONE
		? refreshIndex : LRPerceptionRules::SelectSlot(EchoSlots, now);
	if (!EchoSlots.IsValidIndex(slotIndex))
	{
		return;
	}
	SetEchoSlot(slotIndex, request, now, refreshIndex != INDEX_NONE);
	WriteRuntimeParameters();
	SpawnPulseVFX(request);
}

void ULRPerceptionPresentationComponent::SpawnPulseVFX(const FLRPerceptionPulseRequest& request)
{
	if (IsRunningDedicatedServer() || !GetWorld() || !Tuning)
	{
		return;
	}
	TSoftObjectPtr<UNiagaraSystem> systemReference = Tuning->PerceptionPulseSystem;
	if (const ULRPerceptionSoundSourceComponent* source = Cast<ULRPerceptionSoundSourceComponent>(request.SourceObject.Get()))
	{
		if (!source->GetNiagaraOverride().IsNull())
		{
			systemReference = source->GetNiagaraOverride();
		}
	}
	UNiagaraSystem* system = systemReference.LoadSynchronous();
	if (!system)
	{
		return;
	}
	UNiagaraComponent* component = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, system,
		request.WorldLocation, FRotator::ZeroRotator, FVector::OneVector, true, true,
		ENCPoolMethod::AutoRelease, true);
	if (!component)
	{
		return;
	}
	component->SetVariableFloat(FName(TEXT("User.Radius")), ResolveVisualRadius(request, Tuning));
	component->SetVariableFloat(FName(TEXT("User.ExpansionSeconds")), Tuning->EchoExpansionSeconds);
	component->SetVariableFloat(FName(TEXT("User.WaveWidthCm")), Tuning->EchoWaveWidthCm);
	component->SetVariableFloat(FName(TEXT("User.Intensity")), request.Intensity);
	if (VisualStyle)
	{
		component->SetVariableLinearColor(FName(TEXT("User.Tint")), VisualStyle->WetTint);
	}
}

float ULRPerceptionPresentationComponent::GetCurrentGameTime() const
{
	return GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
}

int32 ULRPerceptionPresentationComponent::FindRefreshSlot(const FLRPerceptionPulseRequest& request,
	const float now) const
{
	if (!request.bRefreshExistingSource)
	{
		return INDEX_NONE;
	}
	const float mergeDistance = Tuning ? Tuning->EchoRefreshMergeDistanceCm : 25.0f;
	for (int32 index = 0; index < EchoSlots.Num(); ++index)
	{
		if (LRPerceptionRules::CanRefreshSlot(EchoSlots[index], request, now, mergeDistance))
		{
			return index;
		}
	}
	return INDEX_NONE;
}

void ULRPerceptionPresentationComponent::SetEchoSlot(const int32 slotIndex,
	const FLRPerceptionPulseRequest& request, const float now, const bool bRefresh)
{
	FLRPerceptionEchoSlot& slot = EchoSlots[slotIndex];
	const float radius = ResolveVisualRadius(request, Tuning);
	const float duration = Tuning ? Tuning->NoiseRevealDurationSeconds : 5.0f;
	if (!bRefresh)
	{
		slot = FLRPerceptionEchoSlot();
		slot.Center = request.WorldLocation;
		slot.FirstStartTime = now;
		slot.SourceObject = request.SourceObject;
	}
	slot.bActive = true;
	slot.RadiusCm = radius;
	slot.LastPulseTime = now;
	slot.ExpireTime = now + FMath::Max(duration, 0.001f);
	slot.Intensity = FMath::Clamp(request.Intensity, 0.0f, 1.0f);
}

void ULRPerceptionPresentationComponent::HandlePerceptionPulse(const FLRPerceptionPulseRequest& request)
{
	ApplyPulse(request);
}
