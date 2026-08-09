#include "Framework/LRPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Framework/LRCharacter.h"
#include "Gameplay/LRLocomotionComponent.h"
#include "InputAction.h"
#include "Input/LRInputConfig.h"
#include "InputMappingContext.h"
#include "Data/LRProjectSettings.h"

void ALRPlayerController::BeginPlay()
{
	Super::BeginPlay();
	SetLRInputMode(InputMode);
}

void ALRPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (!InputConfig)
	{
		InputConfig = GetDefault<ULRProjectSettings>()->InputConfig.LoadSynchronous();
	}
	UEnhancedInputComponent* enhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
	if (!ensureMsgf(enhancedInput && InputConfig, TEXT("%s requires Enhanced Input and an InputConfig."), *GetNameSafe(this)))
	{
		return;
	}

	enhancedInput->BindAction(InputConfig->MoveAction, ETriggerEvent::Triggered, this, &ALRPlayerController::HandleMove);
	enhancedInput->BindAction(InputConfig->ToggleCrouchAction, ETriggerEvent::Started, this, &ALRPlayerController::HandleSneakToggle);
	enhancedInput->BindAction(InputConfig->RunAction, ETriggerEvent::Started, this, &ALRPlayerController::HandleRunStarted);
	enhancedInput->BindAction(InputConfig->RunAction, ETriggerEvent::Completed, this, &ALRPlayerController::HandleRunStopped);
	enhancedInput->BindAction(InputConfig->RunAction, ETriggerEvent::Canceled, this, &ALRPlayerController::HandleRunStopped);
}

void ALRPlayerController::SetLRInputMode(const ELRInputMode newMode)
{
	const ELRInputMode previousMode = InputMode;
	InputMode = newMode;
	UEnhancedInputLocalPlayerSubsystem* subsystem = GetLocalPlayer()
		? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()) : nullptr;
	UInputMappingContext* context = ResolveContext(newMode);
	if (!subsystem || !context)
	{
		return;
	}

	subsystem->ClearAllMappings();
	FModifyContextOptions options;
	options.bIgnoreAllPressedKeysUntilRelease = true;
	subsystem->AddMappingContext(context, static_cast<int32>(newMode), options);
	if (previousMode != newMode)
	{
		OnInputModeChanged.Broadcast(previousMode, newMode);
	}
}

void ALRPlayerController::HandleMove(const FInputActionValue& value)
{
	if (ALRCharacter* character = Cast<ALRCharacter>(GetPawn()))
	{
		character->ApplyMoveInput(value.Get<FVector2D>());
	}
}

void ALRPlayerController::HandleSneakToggle()
{
	if (ALRCharacter* character = Cast<ALRCharacter>(GetPawn()))
	{
		character->GetLocomotionComponent()->ToggleSneak();
	}
}

void ALRPlayerController::HandleRunStarted()
{
	if (ALRCharacter* character = Cast<ALRCharacter>(GetPawn()))
	{
		character->GetLocomotionComponent()->StartRun();
	}
}

void ALRPlayerController::HandleRunStopped()
{
	if (ALRCharacter* character = Cast<ALRCharacter>(GetPawn()))
	{
		character->GetLocomotionComponent()->StopRun();
	}
}

UInputMappingContext* ALRPlayerController::ResolveContext(const ELRInputMode mode) const
{
	if (!InputConfig)
	{
		return nullptr;
	}
	if (mode == ELRInputMode::Dialogue)
	{
		return InputConfig->DialogueContext;
	}
	if (mode == ELRInputMode::Menu)
	{
		return InputConfig->MenuContext;
	}
	if (mode == ELRInputMode::Transition)
	{
		return InputConfig->TransitionContext;
	}
	return InputConfig->GameplayContext;
}
