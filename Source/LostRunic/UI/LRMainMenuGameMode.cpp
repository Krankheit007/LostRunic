#include "UI/LRMainMenuGameMode.h"

#include "Framework/LRPlayerController.h"
#include "UI/LRMainMenuHUD.h"

ALRMainMenuGameMode::ALRMainMenuGameMode()
{
	DefaultPawnClass = nullptr;
	PlayerControllerClass = ALRPlayerController::StaticClass();
	HUDClass = ALRMainMenuHUD::StaticClass();
}
