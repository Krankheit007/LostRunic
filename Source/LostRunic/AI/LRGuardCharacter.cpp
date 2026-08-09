#include "AI/LRGuardCharacter.h"

#include "AI/LRAlertComponent.h"
#include "AI/LRGuardAIController.h"
#include "Core/LRGameplayTags.h"
#include "Engine/GameInstance.h"
#include "Framework/LRCharacter.h"
#include "Items/LRCourageResponseComponent.h"
#include "Save/LRSaveSubsystem.h"
#include "State/LRStateComponent.h"

ALRGuardCharacter::ALRGuardCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	AIControllerClass = ALRGuardAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	Alert = CreateDefaultSubobject<ULRAlertComponent>(TEXT("Alert"));
	CourageResponse = CreateDefaultSubobject<ULRCourageResponseComponent>(TEXT("CourageResponse"));
}

bool ALRGuardCharacter::CaptureTarget(AActor* target)
{
	ULRStateComponent* state = target ? target->FindComponentByClass<ULRStateComponent>() : nullptr;
	if (!state)
	{
		return false;
	}
	FLRStateChangeRequest request;
	request.TargetMode = ELRPerceptionMode::Memory;
	request.RequestType = ELRStateRequestType::Death;
	request.Source = LRGameplayTags::StateSourceDeath;
	const FLRStateChangeResult result = state->RequestStateChange(request);
	if (result.bAccepted)
	{
		if (ALRCharacter* character = Cast<ALRCharacter>(target))
		{
			if (UGameInstance* gameInstance = character->GetGameInstance())
			{
				if (ULRSaveSubsystem* saveSubsystem = gameInstance->GetSubsystem<ULRSaveSubsystem>())
				{
					saveSubsystem->BeginDeathMemoryTransaction(character);
				}
			}
		}
		OnPlayerCaptured.Broadcast(target);
	}
	return result.bAccepted;
}

AActor* ALRGuardCharacter::GetPatrolPoint(const int32 index) const
{
	return PatrolPoints.IsValidIndex(index) ? PatrolPoints[index] : nullptr;
}
