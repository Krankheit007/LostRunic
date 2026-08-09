#pragma once

#include "GameFramework/GameStateBase.h"

#include "LRGameState.generated.h"

/** Runtime game state for the single-player LostRunic experience. */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Game State"))
class LOSTRUNIC_API ALRGameState : public AGameStateBase
{
	GENERATED_BODY()
};
