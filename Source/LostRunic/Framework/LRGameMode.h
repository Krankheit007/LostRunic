#pragma once

#include "GameFramework/GameModeBase.h"

#include "LRGameMode.generated.h"

/** Framework assembly root for LostRunic maps. */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Game Mode"))
class LOSTRUNIC_API ALRGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ALRGameMode();
};
