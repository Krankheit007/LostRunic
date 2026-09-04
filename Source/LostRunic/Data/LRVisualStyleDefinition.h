/**
 * @file LRVisualStyleDefinition.h
 * @brief Authored Perception world appearance, kept separate from state eye-overlay animation style.
 */
#pragma once

#include "Data/LRTuningAsset.h"

#include "LRVisualStyleDefinition.generated.h"

class UTexture2D;

/** Palette and surface shaping values consumed by the Perception post-process material. */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Visual Style Definition"))
class LOSTRUNIC_API ULRVisualStyleDefinition : public ULRTuningAsset
{
	GENERATED_BODY()

public:
	/** Four-row palette: Player, Echo residue, Wet wave, Narrative Accent. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perception|Palette")
	TObjectPtr<UTexture2D> Palette;

	/** Output for pixels with no valid visible scene surface or no reveal. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perception|Palette")
	FLinearColor BlindColor = FLinearColor::Black;

	/** Minimum value applied only to an already revealed surface. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perception|Palette", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RevealedValueFloor = 0.075f;

	/** Shoulder term used before palette mapping. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perception|HDR", meta = (ClampMin = "0.001", ClampMax = "100.0"))
	float ValueShoulder = 1.0f;

	/** Value scale used before palette mapping. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perception|HDR", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float ValueScale = 1.0f;

	/** Value bias used before palette mapping. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perception|HDR", meta = (ClampMin = "-100.0", ClampMax = "100.0"))
	float ValueBias = 0.0f;

	/** Gamma applied after HDR shoulder compression. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perception|HDR", meta = (ClampMin = "0.001", ClampMax = "10.0"))
	float ValueGamma = 1.0f;

	/** Fraction of compressed scene hue retained in the Player base color. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perception|Surface", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float NormalColorRetention = 0.20f;

	/** Minimum directional form contribution on revealed surfaces. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perception|Surface", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ShapeLiftStrength = 0.10f;

	/** Direction used by the Half-Lambert shape lift. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perception|Surface")
	FVector ShapeDirection = FVector(0.0f, 0.0f, 1.0f);

	/** Tint mixed over the historical Echo residue contribution. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perception|Echo")
	FLinearColor EchoTint = FLinearColor(0.35f, 0.75f, 1.0f, 1.0f);

	/** Tint mixed over the latest wet wave contribution. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perception|Echo")
	FLinearColor WetTint = FLinearColor(0.60f, 0.90f, 1.0f, 1.0f);

	/** Tint mixed over a visible Stencil 3 Narrative Accent. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perception|Accent")
	FLinearColor NarrativeAccentTint = FLinearColor(1.0f, 0.75f, 0.30f, 1.0f);

	/** Validates authored ranges; asset references remain optional for safe CDO fallback. */
	virtual bool Validate(FString& outError) const override;
};
