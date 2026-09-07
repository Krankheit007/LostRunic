/**
 * @file LRPerceptionPresentationComponent_Materials.cpp
 * @brief Writes Perception post-process MID and MPC parameters.
 */
#include "Perception/LRPerceptionPresentationComponent.h"

#include "Camera/CameraComponent.h"
#include "Data/LRPresentationTuning.h"
#include "Data/LRVisualStyleDefinition.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Perception/LRPerceptionMaterialParameters.h"

void ULRPerceptionPresentationComponent::AddBlendable()
{
	if (Camera && PerceptionPostProcessMID)
	{
		Camera->AddOrUpdateBlendable(PerceptionPostProcessMID, Tuning ? Tuning->PerceptionBlendWeight : 1.0f);
	}
}

void ULRPerceptionPresentationComponent::RemoveBlendable()
{
	if (Camera && PerceptionPostProcessMID)
	{
		Camera->RemoveBlendable(PerceptionPostProcessMID);
	}
}

void ULRPerceptionPresentationComponent::WriteStyleParameters()
{
	if (PerceptionPostProcessMID && VisualStyle)
	{
		const float fullRevealRadius = Tuning ? Tuning->PerceptionFullRevealRadius : 400.0f;
		const float colorFullRadius = Tuning ? Tuning->PerceptionColorFullRadius : 280.0f;
		const float internalEdgeFullRadius = Tuning ? Tuning->PerceptionInternalEdgeFullRadius : 340.0f;
		const float revealRadius = Tuning ? Tuning->PerceptionRevealRadius : 450.0f;
		const float expansionSeconds = Tuning ? Tuning->EchoExpansionSeconds : 0.75f;
		const float wetSeconds = Tuning ? Tuning->EchoWetSeconds : 0.20f;
		const float dryFadeDurationSeconds = Tuning ? Tuning->EchoDryFadeDurationSeconds : 1.30f;
		const float waveWidthCm = Tuning ? Tuning->EchoWaveWidthCm : 12.5f;
		const float boundaryNoiseCm = Tuning ? Tuning->PerceptionBoundaryNoiseCm : 18.0f;
		PerceptionPostProcessMID->SetTextureParameterValue(FName(LRPerceptionMaterialParameters::Palette), VisualStyle->Palette);
		PerceptionPostProcessMID->SetVectorParameterValue(FName(LRPerceptionMaterialParameters::BlindColor), VisualStyle->BlindColor);
		PerceptionPostProcessMID->SetScalarParameterValue(FName(LRPerceptionMaterialParameters::RevealedValueFloor), VisualStyle->RevealedValueFloor);
		PerceptionPostProcessMID->SetScalarParameterValue(FName(LRPerceptionMaterialParameters::ValueShoulder), VisualStyle->ValueShoulder);
		PerceptionPostProcessMID->SetScalarParameterValue(FName(LRPerceptionMaterialParameters::ValueScale), VisualStyle->ValueScale);
		PerceptionPostProcessMID->SetScalarParameterValue(FName(LRPerceptionMaterialParameters::ValueBias), VisualStyle->ValueBias);
		PerceptionPostProcessMID->SetScalarParameterValue(FName(LRPerceptionMaterialParameters::ValueGamma), VisualStyle->ValueGamma);
		PerceptionPostProcessMID->SetScalarParameterValue(FName(LRPerceptionMaterialParameters::NormalColorRetention), VisualStyle->NormalColorRetention);
		PerceptionPostProcessMID->SetScalarParameterValue(FName(LRPerceptionMaterialParameters::ShapeLiftStrength), VisualStyle->ShapeLiftStrength);
		PerceptionPostProcessMID->SetVectorParameterValue(FName(LRPerceptionMaterialParameters::ShapeDirection),
			FLinearColor(VisualStyle->ShapeDirection.X, VisualStyle->ShapeDirection.Y, VisualStyle->ShapeDirection.Z, 0.0f));
		PerceptionPostProcessMID->SetVectorParameterValue(FName(LRPerceptionMaterialParameters::InkEdgeTint), VisualStyle->InkEdgeTint);
		PerceptionPostProcessMID->SetVectorParameterValue(FName(LRPerceptionMaterialParameters::EchoTint), VisualStyle->EchoTint);
		PerceptionPostProcessMID->SetVectorParameterValue(FName(LRPerceptionMaterialParameters::WetTint), VisualStyle->WetTint);
		PerceptionPostProcessMID->SetVectorParameterValue(FName(LRPerceptionMaterialParameters::NarrativeAccentTint), VisualStyle->NarrativeAccentTint);
		PerceptionPostProcessMID->SetScalarParameterValue(FName(LRPerceptionMaterialParameters::AccentDepthToleranceCm),
			Tuning ? Tuning->AccentDepthToleranceCm : 3.0f);
		PerceptionPostProcessMID->SetScalarParameterValue(FName(LRPerceptionMaterialParameters::DebugView),
			static_cast<float>(DebugView));
		PerceptionPostProcessMID->SetScalarParameterValue(FName(LRPerceptionMaterialParameters::PerceptionFullRevealRadius), fullRevealRadius);
		PerceptionPostProcessMID->SetScalarParameterValue(FName(LRPerceptionMaterialParameters::PerceptionColorFullRadius), colorFullRadius);
		PerceptionPostProcessMID->SetScalarParameterValue(FName(LRPerceptionMaterialParameters::PerceptionInternalEdgeFullRadius), internalEdgeFullRadius);
		PerceptionPostProcessMID->SetScalarParameterValue(FName(LRPerceptionMaterialParameters::PerceptionRevealRadius), revealRadius);
		PerceptionPostProcessMID->SetScalarParameterValue(FName(LRPerceptionMaterialParameters::EchoExpansionSeconds), expansionSeconds);
		PerceptionPostProcessMID->SetScalarParameterValue(FName(LRPerceptionMaterialParameters::EchoWetSeconds), wetSeconds);
		PerceptionPostProcessMID->SetScalarParameterValue(FName(LRPerceptionMaterialParameters::EchoDryFadeDurationSeconds), dryFadeDurationSeconds);
		PerceptionPostProcessMID->SetScalarParameterValue(FName(LRPerceptionMaterialParameters::EchoWaveWidthCm), waveWidthCm);
		PerceptionPostProcessMID->SetScalarParameterValue(FName(LRPerceptionMaterialParameters::PerceptionBoundaryNoiseCm), boundaryNoiseCm);
	}
	if (VisualStyleMPCInstance)
	{
		VisualStyleMPCInstance->SetScalarParameterValue(FName(LRPerceptionMaterialParameters::StateBlend), CurrentBlend);
		VisualStyleMPCInstance->SetScalarParameterValue(FName(LRPerceptionMaterialParameters::PerceptionIntensity), CurrentBlend);
		VisualStyleMPCInstance->SetScalarParameterValue(FName(LRPerceptionMaterialParameters::NormalOutlineGate), 1.0f - CurrentBlend);
		VisualStyleMPCInstance->SetScalarParameterValue(FName(LRPerceptionMaterialParameters::InteractionPresentationGate),
			bPerceptionActive ? 0.0f : 1.0f);
		VisualStyleMPCInstance->SetVectorParameterValue(FName(LRPerceptionMaterialParameters::PlayerOcclusionColor),
			Tuning ? Tuning->PlayerOcclusionStateColor : FLinearColor(0.35f, 0.85f, 1.0f, 1.0f));
	}
}

void ULRPerceptionPresentationComponent::WriteRuntimeParameters()
{
	if (!RuntimeMPCInstance)
	{
		return;
	}
	RuntimeMPCInstance->SetVectorParameterValue(FName(LRPerceptionMaterialParameters::PlayerPosition),
		FLinearColor(PlayerPosition.X, PlayerPosition.Y, PlayerPosition.Z, 1.0f));
	for (int32 index = 0; index < MaxEchoSlots; ++index)
	{
		const FLRPerceptionEchoSlot* slot = EchoSlots.IsValidIndex(index) ? &EchoSlots[index] : nullptr;
		const FLinearColor centerRadius = slot && slot->bActive
			? FLinearColor(slot->Center.X, slot->Center.Y, slot->Center.Z, slot->RadiusCm)
			: FLinearColor::Transparent;
		const FLinearColor timing = slot && slot->bActive
			? FLinearColor(slot->FirstStartTime, slot->LastPulseTime, slot->ExpireTime, slot->Intensity)
			: FLinearColor::Transparent;
		RuntimeMPCInstance->SetVectorParameterValue(LRPerceptionMaterialParameters::EchoCenterRadius(index), centerRadius);
		RuntimeMPCInstance->SetVectorParameterValue(LRPerceptionMaterialParameters::EchoTiming(index), timing);
	}
}
