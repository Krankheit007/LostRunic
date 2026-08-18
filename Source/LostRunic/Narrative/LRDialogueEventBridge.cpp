/** @file LRDialogueEventBridge.cpp @brief Dispatches SUDS event namespaces into LostRunic systems. */
#include "Narrative/LRDialogueEventBridge.h"

#include "Core/LRLog.h"
#include "Narrative/LRStoryStateSubsystem.h"
#include "SUDSDialogue.h"

void ULRDialogueEventBridge::HandleDialogueEvent(USUDSDialogue* Dialogue, const FName EventName, const TArray<FSUDSValue>& Arguments)
{
	if (!Dialogue)
	{
		return;
	}
	const FString Name = EventName.ToString();
	if (Name.StartsWith(TEXT("Story.")))
	{
		const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(EventName, false);
		if (!Tag.IsValid() || !StoryState.IsValid())
		{
			UE_LOG(LogLostRunicNarrative, Warning, TEXT("SUDS Story event=%s is not a registered Story tag."), *Name);
			return;
		}
		StoryState->AddStoryFlag(Tag);
		return;
	}
	if (Name.StartsWith(TEXT("Save.")))
	{
		UE_LOG(LogLostRunicNarrative, Verbose, TEXT("SUDS requested Save event=%s args=%d."), *Name, Arguments.Num());
		return;
	}
	UE_LOG(LogLostRunicNarrative, Warning, TEXT("Unhandled SUDS event namespace event=%s."), *Name);
}
