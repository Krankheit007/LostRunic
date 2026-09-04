/**
 * @file LRPerceptionMaterialParameters.h
 * @brief Stable names shared by the Perception material and its runtime writers.
 */
#pragma once

namespace LRPerceptionMaterialParameters
{
	inline constexpr TCHAR StateBlend[] = TEXT("LR_StateBlend");
	inline constexpr TCHAR PerceptionIntensity[] = TEXT("LR_PerceptionIntensity");
	inline constexpr TCHAR NormalOutlineGate[] = TEXT("LR_NormalOutlineGate");
	inline constexpr TCHAR InteractionPresentationGate[] = TEXT("LR_InteractionPresentationGate");
	inline constexpr TCHAR PlayerOcclusionColor[] = TEXT("LR_PlayerOcclusionColor");
	inline constexpr TCHAR PlayerPosition[] = TEXT("LR_PlayerPosition");
	inline constexpr TCHAR EchoCenterRadiusPrefix[] = TEXT("LR_EchoCenterRadius");
	inline constexpr TCHAR EchoTimingPrefix[] = TEXT("LR_EchoTiming");
	inline constexpr TCHAR Palette[] = TEXT("LR_Palette");
	inline constexpr TCHAR BlindColor[] = TEXT("LR_BlindColor");
	inline constexpr TCHAR RevealedValueFloor[] = TEXT("LR_RevealedValueFloor");
	inline constexpr TCHAR ValueShoulder[] = TEXT("LR_ValueShoulder");
	inline constexpr TCHAR ValueScale[] = TEXT("LR_ValueScale");
	inline constexpr TCHAR ValueBias[] = TEXT("LR_ValueBias");
	inline constexpr TCHAR ValueGamma[] = TEXT("LR_ValueGamma");
	inline constexpr TCHAR NormalColorRetention[] = TEXT("LR_NormalColorRetention");
	inline constexpr TCHAR ShapeLiftStrength[] = TEXT("LR_ShapeLiftStrength");
	inline constexpr TCHAR ShapeDirection[] = TEXT("LR_ShapeDirection");
	inline constexpr TCHAR EchoTint[] = TEXT("LR_EchoTint");
	inline constexpr TCHAR WetTint[] = TEXT("LR_WetTint");
	inline constexpr TCHAR NarrativeAccentTint[] = TEXT("LR_NarrativeAccentTint");
	inline constexpr TCHAR AccentDepthToleranceCm[] = TEXT("LR_AccentDepthToleranceCm");
	inline constexpr TCHAR DebugView[] = TEXT("LR_DebugView");

	inline FName EchoCenterRadius(const int32 slotIndex)
	{
		return FName(*FString::Printf(TEXT("%s%d"), EchoCenterRadiusPrefix, slotIndex));
	}

	inline FName EchoTiming(const int32 slotIndex)
	{
		return FName(*FString::Printf(TEXT("%s%d"), EchoTimingPrefix, slotIndex));
	}
}
