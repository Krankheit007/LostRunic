#include "Save/LRSaveRequestQueue.h"

void LRSaveRequestQueue::Enqueue(TArray<FLRQueuedSaveRequest>& queue, FLRQueuedSaveRequest&& request)
{
	queue.Add(MoveTemp(request));
}

bool LRSaveRequestQueue::Dequeue(TArray<FLRQueuedSaveRequest>& queue, FLRQueuedSaveRequest& outRequest)
{
	if (queue.IsEmpty())
	{
		return false;
	}
	outRequest = MoveTemp(queue[0]);
	queue.RemoveAt(0);
	return true;
}
