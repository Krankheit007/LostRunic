/** @file LRMainMenuGameMode.h @brief 主菜单 Host GameMode：无 Pawn、复用通用 PlayerController。 */
#pragma once

#include "GameFramework/GameModeBase.h"

#include "LRMainMenuGameMode.generated.h"

UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Main Menu Game Mode"))
class LOSTRUNIC_API ALRMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ALRMainMenuGameMode();
};
