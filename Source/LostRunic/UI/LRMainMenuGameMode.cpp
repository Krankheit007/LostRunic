#include "UI/LRMainMenuGameMode.h"

#include "Framework/LRPlayerController.h"
#include "UI/LRMainMenuHUD.h"

ALRMainMenuGameMode::ALRMainMenuGameMode()
{
	DefaultPawnClass = nullptr;
	PlayerControllerClass = ALRPlayerController::StaticClass();
	HUDClass = ALRMainMenuHUD::StaticClass();
}

AActor* ALRMainMenuGameMode::FindPlayerStart_Implementation(AController* player, const FString& incomingName)
{
	// Login requires a non-null start actor even though RestartPlayer is intentionally suppressed.
	(void)player;
	(void)incomingName;
	return this;
}

void ALRMainMenuGameMode::RestartPlayer(AController* newPlayer)
{
	// The main-menu controller owns UI input only and intentionally has no Pawn or PlayerStart.
	(void)newPlayer;
}
