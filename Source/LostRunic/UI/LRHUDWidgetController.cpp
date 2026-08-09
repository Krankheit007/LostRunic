#include "UI/LRHUDWidgetController.h"

#include "Framework/LRCharacter.h"
#include "State/LRStateComponent.h"

void ULRHUDWidgetController::SetObservedCharacter(ALRCharacter* character)
{
	Deinitialize();
	ObservedCharacter = character;
	ULRStateComponent* state = character ? character->GetStateComponent() : nullptr;
	if (!state)
	{
		return;
	}
	CurrentMode = state->GetCurrentMode();
	state->OnStateChanged.AddDynamic(this, &ULRHUDWidgetController::HandleStateChanged);
}

void ULRHUDWidgetController::Deinitialize()
{
	if (ALRCharacter* character = ObservedCharacter.Get())
	{
		character->GetStateComponent()->OnStateChanged.RemoveDynamic(this, &ULRHUDWidgetController::HandleStateChanged);
	}
	ObservedCharacter.Reset();
	CurrentMode = ELRPerceptionMode::Normal;
}

void ULRHUDWidgetController::HandleStateChanged(const ELRPerceptionMode currentMode, const FGameplayTag reason)
{
	CurrentMode = currentMode;
	OnPerceptionModeChanged.Broadcast(CurrentMode, reason);
}
