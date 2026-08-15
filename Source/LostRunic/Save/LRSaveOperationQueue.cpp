#include "Save/LRSaveOperationQueue.h"

void LRSaveOperationQueue::Enqueue(TArray<FLRQueuedSaveOperation>& queue, FLRQueuedSaveOperation&& operation)
{
	check(operation.OperationId.IsValid());
	queue.Add(MoveTemp(operation));
}

void LRSaveOperationQueue::EnqueueFront(TArray<FLRQueuedSaveOperation>& queue,
	FLRQueuedSaveOperation&& operation)
{
	check(operation.OperationId.IsValid());
	queue.Insert(MoveTemp(operation), 0);
}

bool LRSaveOperationQueue::Dequeue(TArray<FLRQueuedSaveOperation>& queue, FLRQueuedSaveOperation& outOperation)
{
	if (queue.IsEmpty())
	{
		return false;
	}
	outOperation = MoveTemp(queue[0]);
	queue.RemoveAt(0);
	return true;
}

bool LRSaveOperationQueue::HasCatalogTransaction(const FLRQueuedSaveOperation& activeOperation,
	const ELRSaveOperationState operationState)
{
	return activeOperation.OperationId.IsValid() && (operationState == ELRSaveOperationState::WritingPayload
		|| operationState == ELRSaveOperationState::CommittingCatalog
		|| operationState == ELRSaveOperationState::DeletingPayload);
}
