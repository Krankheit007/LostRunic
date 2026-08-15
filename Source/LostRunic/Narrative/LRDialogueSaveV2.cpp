/** @file LRDialogueSaveV2.cpp @brief V2 save adapter for narrative progress. */
#include "Narrative/LRDialogueSubsystem.h"

void ULRDialogueSubsystem::CaptureStorySaveState(FLRSaveStoryChunk& outStory) const
{
	outStory.CompletedEventIds = CompletedEventIds;
}

void ULRDialogueSubsystem::RestoreStorySaveState(const FLRSaveStoryChunk& savedStory)
{
	CompletedEventIds = savedStory.CompletedEventIds;
}

void ULRDialogueSubsystem::ResetForNewGame()
{
	ResetSession();
	CompletedEventIds.Reset();
}
