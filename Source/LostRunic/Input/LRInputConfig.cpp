#include "Input/LRInputConfig.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

bool ULRInputConfig::Validate(FString& outError) const
{
	if (!GameplayContext || !DialogueContext || !MenuContext || !TransitionContext)
	{
		outError = TEXT("All four input mapping contexts are required.");
		return false;
	}
	if (!MoveAction || !SneakAction || !RunAction || !InteractAction || !CloseEyesAction || !OpenEyesAction)
	{
		outError = TEXT("Gameplay movement, interaction, and state actions are required.");
		return false;
	}
	if (!ConfirmAction || !CancelAction || !UseQuickSlotAction || !PreviousQuickSlotAction || !NextQuickSlotAction
		|| !ToggleCrouchAction || !OpenJournalAction || !PauseAction
		|| QuickSlotActions.Num() != 4 || QuickSlotActions.Contains(nullptr))
	{
		outError = TEXT("UI and all four quick-slot actions are required.");
		return false;
	}
	return true;
}

#if WITH_EDITOR
EDataValidationResult ULRInputConfig::IsDataValid(FDataValidationContext& context) const
{
	FString error;
	if (!Validate(error))
	{
		context.AddError(FText::FromString(error));
		return EDataValidationResult::Invalid;
	}
	return Super::IsDataValid(context);
}
#endif
