#include "Framework/LRPlayerController.h"

#include "Core/LRGameplayTags.h"
#include "Core/LRLog.h"
#include "Data/LRProjectSettings.h"
#include "Framework/LRCharacter.h"
#include "Gameplay/LRLocomotionComponent.h"
#include "Input/LRInputConfig.h"
#include "Interaction/LRInteractionComponent.h"
#include "Items/LRInventoryComponent.h"
#include "State/LRStateComponent.h"
#include "UI/LRPlayerUIComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"

ALRPlayerController::ALRPlayerController()
{
	PlayerUI = CreateDefaultSubobject<ULRPlayerUIComponent>(TEXT("PlayerUI"));
}

void ALRPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!InputConfig)
	{
		InputConfig = GetDefault<ULRProjectSettings>()->InputConfig.LoadSynchronous();
	}
	if (PlayerUI)
	{
		PlayerUI->InitializeUI(this);
	}
	SetLRInputMode(InputMode);
}

void ALRPlayerController::OnPossess(APawn* pawn)
{
	Super::OnPossess(pawn);
	if (PlayerUI)
	{
		PlayerUI->SetObservedCharacter(Cast<ALRCharacter>(pawn));
	}
}

void ALRPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (!InputConfig)
	{
		InputConfig = GetDefault<ULRProjectSettings>()->InputConfig.LoadSynchronous();
	}
	UEnhancedInputComponent* enhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
	FString inputError;
	const bool bInputValid = InputConfig && InputConfig->Validate(inputError);
	if (!ensureMsgf(enhancedInput && bInputValid, TEXT("%s has invalid input configuration: %s"),
		*GetNameSafe(this), *inputError))
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
	enhancedInput->BindAction(InputConfig->InteractAction, ETriggerEvent::Started, this, &ALRPlayerController::HandleInteract);
	enhancedInput->BindAction(InputConfig->QuickSlotActions[0], ETriggerEvent::Started, this, &ALRPlayerController::HandleQuickSlot1);
	enhancedInput->BindAction(InputConfig->QuickSlotActions[1], ETriggerEvent::Started, this, &ALRPlayerController::HandleQuickSlot2);
	enhancedInput->BindAction(InputConfig->QuickSlotActions[2], ETriggerEvent::Started, this, &ALRPlayerController::HandleQuickSlot3);
	enhancedInput->BindAction(InputConfig->QuickSlotActions[3], ETriggerEvent::Started, this, &ALRPlayerController::HandleQuickSlot4);
	enhancedInput->BindAction(InputConfig->UseQuickSlotAction, ETriggerEvent::Started, this, &ALRPlayerController::HandleUseSelectedQuickSlot);
	enhancedInput->BindAction(InputConfig->PreviousQuickSlotAction, ETriggerEvent::Started, this, &ALRPlayerController::HandlePreviousQuickSlot);
	enhancedInput->BindAction(InputConfig->NextQuickSlotAction, ETriggerEvent::Started, this, &ALRPlayerController::HandleNextQuickSlot);
	enhancedInput->BindAction(InputConfig->ConfirmAction, ETriggerEvent::Started, this, &ALRPlayerController::HandleConfirm);
	enhancedInput->BindAction(InputConfig->CancelAction, ETriggerEvent::Started, this, &ALRPlayerController::HandleCancel);
	enhancedInput->BindAction(InputConfig->OpenJournalAction, ETriggerEvent::Started, this, &ALRPlayerController::HandleOpenJournal);
	enhancedInput->BindAction(InputConfig->PauseAction, ETriggerEvent::Started, this, &ALRPlayerController::HandlePause);
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
		UE_LOG(LogLostRunicUI, Warning, TEXT("Controller=%s cannot apply input mode=%d; missing mapping context."),
			*GetNameSafe(this), static_cast<int32>(newMode));
		return;
	}

	subsystem->ClearAllMappings();
	FModifyContextOptions options;
	options.bIgnoreAllPressedKeysUntilRelease = true;
	subsystem->AddMappingContext(context, static_cast<int32>(newMode), options);
	ConfigureViewportInput(newMode);
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

void ALRPlayerController::HandleInteract()
{
	if (const ALRCharacter* character = Cast<ALRCharacter>(GetPawn()))
	{
		character->GetInteractionComponent()->PerformPrimaryInteraction();
	}
}

void ALRPlayerController::HandleQuickSlot1() { UseQuickSlot(0); }
void ALRPlayerController::HandleQuickSlot2() { UseQuickSlot(1); }
void ALRPlayerController::HandleQuickSlot3() { UseQuickSlot(2); }
void ALRPlayerController::HandleQuickSlot4() { UseQuickSlot(3); }

void ALRPlayerController::HandleUseSelectedQuickSlot()
{
	if (const ALRCharacter* character = Cast<ALRCharacter>(GetPawn()))
	{
		UseQuickSlot(character->GetInventoryComponent()->GetSelectedQuickSlot());
	}
}

void ALRPlayerController::HandlePreviousQuickSlot()
{
	if (const ALRCharacter* character = Cast<ALRCharacter>(GetPawn()))
	{
		character->GetInventoryComponent()->SelectAdjacentQuickSlot(-1);
	}
}

void ALRPlayerController::HandleNextQuickSlot()
{
	if (const ALRCharacter* character = Cast<ALRCharacter>(GetPawn()))
	{
		character->GetInventoryComponent()->SelectAdjacentQuickSlot(1);
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
