#include "Save/LRStorySaveAdapter.h"

namespace LRStorySaveAdapter
{
	void ToSaveChunk(const FLRNarrativePersistentState& persistentState, FLRSaveStoryChunk& outStory)
	{
		outStory.StoryFlags = persistentState.StoryFlags;
		outStory.CompletedEventIds = persistentState.CompletedEventIds;
		outStory.MemoryEventIds = persistentState.MemoryEventIds;
	}

	void ToPersistentState(const FLRSaveStoryChunk& storyChunk, FLRNarrativePersistentState& outState)
	{
		outState.StoryFlags = storyChunk.StoryFlags;
		outState.CompletedEventIds = storyChunk.CompletedEventIds;
		outState.MemoryEventIds = storyChunk.MemoryEventIds;
	}

	void ApplyDeltaToSaveChunk(const FLRNarrativePersistentDelta& persistentDelta, FLRSaveStoryChunk& inOutStory)
	{
		for (const FGameplayTag& storyFlag : persistentDelta.AddedStoryFlags)
		{
			inOutStory.StoryFlags.AddTag(storyFlag);
		}
		inOutStory.CompletedEventIds.Append(persistentDelta.AddedCompletedEventIds);
		inOutStory.MemoryEventIds.Append(persistentDelta.AddedMemoryEventIds);
	}

	bool ValidateDeltaAgainstState(const FLRNarrativePersistentState& persistentState,
		const FLRNarrativePersistentDelta& persistentDelta, FString& outError)
	{
		for (const FGameplayTag& flag : persistentDelta.AddedStoryFlags)
		{
			if (!persistentState.StoryFlags.HasTag(flag))
			{
				outError = TEXT("Durable narrative delta contains an uncommitted Story flag.");
				return false;
			}
		}
		for (const FName eventId : persistentDelta.AddedCompletedEventIds)
		{
			if (!persistentState.CompletedEventIds.Contains(eventId))
			{
				outError = TEXT("Durable narrative delta contains an uncommitted completed event.");
				return false;
			}
		}
		for (const FName eventId : persistentDelta.AddedMemoryEventIds)
		{
			if (!persistentState.MemoryEventIds.Contains(eventId))
			{
				outError = TEXT("Durable narrative delta contains an uncommitted Memory event.");
				return false;
			}
		}
		return true;
	}
}
