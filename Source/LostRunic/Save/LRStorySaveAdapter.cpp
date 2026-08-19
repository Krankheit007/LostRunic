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
}
