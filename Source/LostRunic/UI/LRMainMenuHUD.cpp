#include "UI/LRMainMenuHUD.h"

#include "Engine/GameInstance.h"
#include "Save/LRSaveSubsystem.h"

void ALRMainMenuHUD::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	if (MainMenuWidgetController)
	{
		MainMenuWidgetController->Deinitialize();
	}
	Super::EndPlay(endPlayReason);
}

void ALRMainMenuHUD::InitializeForController(ALRPlayerController* playerController)
{
	Super::InitializeForController(playerController);
	if (!MainMenuWidgetController && GetGameInstance())
	{
		MainMenuWidgetController = NewObject<ULRMainMenuWidgetController>(this);
		MainMenuWidgetController->Initialize(GetGameInstance()->GetSubsystem<ULRSaveSubsystem>());
	}
}
