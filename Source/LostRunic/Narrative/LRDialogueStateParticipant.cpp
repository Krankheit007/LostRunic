/** @file LRDialogueStateParticipant.cpp @brief Supplies Story.* variables to SUDS. */
#include "Narrative/LRDialogueStateParticipant.h"

#include "Narrative/LRStoryStateSubsystem.h"
#include "SUDSDialogue.h"

void ULRDialogueStateParticipant::OnDialogueVariableRequested_Implementation(USUDSDialogue* Dialogue, const FName VariableName)
{
	if (!Dialogue || !StoryState.IsValid())
	{
		return;
	}
	const FString Name = VariableName.ToString();
	if (!Name.StartsWith(TEXT("Story.")))
	{
		return;
	}
	const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(VariableName, false);
	Dialogue->SetVariableBoolean(VariableName, StoryState->HasStoryFlag(Tag));
}
