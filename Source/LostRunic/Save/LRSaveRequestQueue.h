#pragma once

#include "Save/LRSaveTypes.h"

/** Small FIFO primitive used by the save subsystem and direct queue-order automation tests. */
namespace LRSaveRequestQueue
{
	LOSTRUNIC_API void Enqueue(TArray<FLRQueuedSaveRequest>& queue, FLRQueuedSaveRequest&& request);
	LOSTRUNIC_API bool Dequeue(TArray<FLRQueuedSaveRequest>& queue, FLRQueuedSaveRequest& outRequest);
}
