#pragma once

#include "CoreMinimal.h"

#include "LRTypes.generated.h"

/** The four player perception modes used by gameplay rules. */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Perception Mode"))
enum class ELRPerceptionMode : uint8
{
	Normal UMETA(DisplayName = "Normal"),
	Perception UMETA(DisplayName = "Perception"),
	Courage UMETA(DisplayName = "Courage"),
	Memory UMETA(DisplayName = "Memory")
};

/** Active player input layer. Higher layers replace lower layers. */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Input Mode"))
enum class ELRInputMode : uint8
{
	Gameplay UMETA(DisplayName = "Gameplay"),
	Dialogue UMETA(DisplayName = "Dialogue"),
	Menu UMETA(DisplayName = "Menu"),
	Transition UMETA(DisplayName = "Transition")
};

/** Player locomotion pace independent of CharacterMovement movement mode. */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Movement Pace"))
enum class ELRMovementPace : uint8
{
	Sneak UMETA(DisplayName = "Sneak"),
	Walk UMETA(DisplayName = "Walk"),
	Run UMETA(DisplayName = "Run")
};

/** Explicit persistence behavior authored on narrative events. */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Save Policy"))
enum class ELRSavePolicy : uint8
{
	None UMETA(DisplayName = "None"),
	AutoOnComplete UMETA(DisplayName = "Auto On Complete"),
	Critical UMETA(DisplayName = "Critical Ordered Save")
};
