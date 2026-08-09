#include "Framework/LRGameMode.h"

#include "Framework/LRCharacter.h"
#include "Framework/LRGameState.h"
#include "Framework/LRPlayerController.h"

ALRGameMode::ALRGameMode()
{
	DefaultPawnClass = ALRCharacter::StaticClass();
	PlayerControllerClass = ALRPlayerController::StaticClass();
	GameStateClass = ALRGameState::StaticClass();
}
