#include "Framework/LRPlayerController.h"

#include "Core/LRGameplayTags.h"
#include "Data/LRProjectSettings.h"
#include "Framework/LRCharacter.h"
#include "Gameplay/LRLocomotionComponent.h"
#include "Input/LRInputConfig.h"
#include "State/LRStateComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"

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
	enhancedInput->BindAction(InputConfig->CloseEyesAction, ETriggerEvent::Started, this, &ALRPlayerController::HandleCloseEyesStarted);
	enhancedInput->BindAction(InputConfig->CloseEyesAction, ETriggerEvent::Completed, this, &ALRPlayerController::HandleCloseEyesStopped);
	enhancedInput->BindAction(InputConfig->CloseEyesAction, ETriggerEvent::Canceled, this, &ALRPlayerController::HandleCloseEyesStopped);
	enhancedInput->BindAction(InputConfig->OpenEyesAction, ETriggerEvent::Started, this, &ALRPlayerController::HandleOpenEyesStarted);
	enhancedInput->BindAction(InputConfig->OpenEyesAction, ETriggerEvent::Completed, this, &ALRPlayerController::HandleOpenEyesStopped);
	enhancedInput->BindAction(InputConfig->OpenEyesAction, ETriggerEvent::Canceled, this, &ALRPlayerController::HandleOpenEyesStopped);
}

void ALRPlayerController::SetLRInputMode(const ELRInputMode newMode)
{
	const ELRInputMode previousMode = InputMode;
	InputMode = newMode;
	UpdateStateInputBlocker(previousMode, newMode);
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

void ALRPlayerController::HandleCloseEyesStarted()
{
	if (const ALRCharacter* character = Cast<ALRCharacter>(GetPawn()))
	{
		character->GetStateComponent()->BeginEyeInput(ELRStateRequestType::CloseEyes);
	}
}

void ALRPlayerController::HandleCloseEyesStopped()
{
	if (const ALRCharacter* character = Cast<ALRCharacter>(GetPawn()))
	{
		character->GetStateComponent()->EndEyeInput(ELRStateRequestType::CloseEyes);
	}
}

void ALRPlayerController::HandleOpenEyesStarted()
{
	if (const ALRCharacter* character = Cast<ALRCharacter>(GetPawn()))
	{
		character->GetStateComponent()->BeginEyeInput(ELRStateRequestType::OpenEyes);
	}
}

void ALRPlayerController::HandleOpenEyesStopped()
{
	if (const ALRCharacter* character = Cast<ALRCharacter>(GetPawn()))
	{
		character->GetStateComponent()->EndEyeInput(ELRStateRequestType::OpenEyes);
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

void ALRPlayerController::UpdateStateInputBlocker(const ELRInputMode previousMode, const ELRInputMode newMode)
{
	ALRCharacter* character = Cast<ALRCharacter>(GetPawn());
	ULRStateComponent* state = character ? character->GetStateComponent() : nullptr;
	if (!state)
	{
		return;
	}

	const auto getBlocker = [](const ELRInputMode mode) -> FGameplayTag
	{
		if (mode == ELRInputMode::Dialogue) return LRGameplayTags::StateBlockerDialogue;
		if (mode == ELRInputMode::Menu) return LRGameplayTags::StateBlockerMenu;
		if (mode == ELRInputMode::Transition) return LRGameplayTags::StateBlockerTransition;
		return FGameplayTag();
	};
	state->SetBlockerActive(getBlocker(previousMode), false);
	state->SetBlockerActive(getBlocker(newMode), true);
}
