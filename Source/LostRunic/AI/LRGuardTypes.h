#pragma once

#include "CoreMinimal.h"

#include "LRGuardTypes.generated.h"

/** Designer-visible guard behavior bands derived from alert state. */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Guard Behavior"))
enum class ELRGuardBehaviorState : uint8
{
	IdlePatrol UMETA(DisplayName = "Idle / Patrol"),
	Suspicious UMETA(DisplayName = "Suspicious"),
	Investigate UMETA(DisplayName = "Investigate"),
	Search UMETA(DisplayName = "Search"),
	Chase UMETA(DisplayName = "Chase")
};
