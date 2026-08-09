#include "Framework/LRGameMode.h"

#include "Framework/LRCharacter.h"
#include "Framework/LRGameState.h"
#include "Framework/LRPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Save/LRSaveSubsystem.h"
#include "UI/LRHUD.h"

ALRGameMode::ALRGameMode()
{
	DefaultPawnClass = ALRCharacter::StaticClass();
	PlayerControllerClass = ALRPlayerController::StaticClass();
	GameStateClass = ALRGameState::StaticClass();
	HUDClass = ALRHUD::StaticClass();
}

void ALRGameMode::BeginPlay()
{
	Super::BeginPlay();
	if (UGameInstance* gameInstance = GetGameInstance())
	{
		if (ULRSaveSubsystem* saveSubsystem = gameInstance->GetSubsystem<ULRSaveSubsystem>())
		{
			saveSubsystem->HandleWorldReady(Cast<ALRCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)));
		}
	}
}
