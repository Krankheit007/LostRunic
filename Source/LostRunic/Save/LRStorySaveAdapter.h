/** @file LRStorySaveAdapter.h @brief Save-layer adapter between narrative persistent state and V2 story chunks. */
#pragma once

#include "Narrative/LRNarrativeTypes.h"
#include "Save/LRSaveV2Types.h"

namespace LRStorySaveAdapter
{
	LOSTRUNIC_API void ToSaveChunk(const FLRNarrativePersistentState& persistentState, FLRSaveStoryChunk& outStory);
	LOSTRUNIC_API void ToPersistentState(const FLRSaveStoryChunk& storyChunk, FLRNarrativePersistentState& outState);
	LOSTRUNIC_API void ApplyDeltaToSaveChunk(const FLRNarrativePersistentDelta& persistentDelta,
		FLRSaveStoryChunk& inOutStory);
}
