/** @file LRDialogueSaveV2.cpp @brief V2 save adapter for narrative progress. */
#include "Narrative/LRDialogueSubsystem.h"

void ULRDialogueSubsystem::CaptureStorySaveState(FLRSaveStoryChunk& outStory) const
{
	outStory.CompletedEventIds = CompletedEventIds;
	outStory.MemoryEventIds = MemoryEventIds;
}

void ULRDialogueSubsystem::RestoreStorySaveState(const FLRSaveStoryChunk& savedStory)
{
	CompletedEventIds = savedStory.CompletedEventIds;
	MemoryEventIds = savedStory.MemoryEventIds;
}

void ULRDialogueSubsystem::CaptureMemoryEventIds(TSet<FName>& outEventIds) const
{
	outEventIds = MemoryEventIds;
}

void ULRDialogueSubsystem::RestoreMemoryEventIds(const TSet<FName>& eventIds)
{
	MemoryEventIds = eventIds;
}

bool ULRDialogueSubsystem::RecordMemoryEvent(const FName eventId)
{
	if (eventId.IsNone() || MemoryEventIds.Contains(eventId))
	{
		return false;
	}
	MemoryEventIds.Add(eventId);
	return true;
}

void ULRDialogueSubsystem::ResetForNewGame()
{
	ResetSession();
	CompletedEventIds.Reset();
	MemoryEventIds.Reset();
}
