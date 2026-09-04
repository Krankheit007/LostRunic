/**
 * @file LRPerceptionTypes.h
 * @brief Runtime contracts shared by the Perception event bus, presentation component and audio sources.
 */
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "LRPerceptionTypes.generated.h"

/** Material inspection views used by the Perception debug scalar. */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Perception Debug View"))
enum class ELRPerceptionDebugView : uint8
{
	None UMETA(DisplayName = "None"),
	Reveal UMETA(DisplayName = "Reveal"),
	RawEdge UMETA(DisplayName = "Raw Edge"),
	GatedEdge UMETA(DisplayName = "Gated Edge"),
	MappedLuma UMETA(DisplayName = "Mapped Luma"),
	EchoSlot UMETA(DisplayName = "Echo Slot"),
	ValidSceneSurface UMETA(DisplayName = "Valid Scene Surface"),
	AccentVisibility UMETA(DisplayName = "Accent Visibility")
};

/** A single source event entering the Perception visual event bus. */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Perception Pulse Request"))
struct LOSTRUNIC_API FLRPerceptionPulseRequest
{
	GENERATED_BODY()

	/** Non-owning source identity used by the spatial refresh contract. */
	UPROPERTY(BlueprintReadWrite, Category = "Perception")
	TWeakObjectPtr<UObject> SourceObject;

	/** World-space origin of this spatial event, in Unreal centimeters. */
	UPROPERTY(BlueprintReadWrite, Category = "Perception", meta = (Units = "cm"))
	FVector WorldLocation = FVector::ZeroVector;

	/** Explicit visual radius; non-positive values resolve to PresentationTuning.NoiseRevealRadius. */
	UPROPERTY(BlueprintReadWrite, Category = "Perception", meta = (Units = "cm"))
	float VisualRadiusCm = 0.0f;

	/** Source intensity consumed by the composite and optional Niagara decoration. */
	UPROPERTY(BlueprintReadWrite, Category = "Perception", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Intensity = 1.0f;

	/** Stable gameplay reason used for diagnostics and source filtering. */
	UPROPERTY(BlueprintReadWrite, Category = "Perception")
	FGameplayTag Reason;

	/** True only for looping sources that intentionally refresh a nearby slot. */
	UPROPERTY(BlueprintReadWrite, Category = "Perception")
	bool bRefreshExistingSource = false;
};

/** Runtime data mirrored into one of the fixed eight Perception MPC slots. */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Perception Echo Slot"))
struct LOSTRUNIC_API FLRPerceptionEchoSlot
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Perception")
	bool bActive = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Perception", meta = (Units = "cm"))
	FVector Center = FVector::ZeroVector;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Perception", meta = (Units = "cm"))
	float RadiusCm = 0.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Perception|Time", meta = (Units = "s"))
	float FirstStartTime = 0.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Perception|Time", meta = (Units = "s"))
	float LastPulseTime = 0.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Perception|Time", meta = (Units = "s"))
	float ExpireTime = 0.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Perception")
	float Intensity = 0.0f;

	/** Non-owning identity; the owning source/component remains responsible for its lifetime. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Perception")
	TWeakObjectPtr<UObject> SourceObject;
};
