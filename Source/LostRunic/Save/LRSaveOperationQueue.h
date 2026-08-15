/** @file LRSaveOperationQueue.h @brief The sole ordering boundary for every V2 persistence operation. */
#pragma once

#include "CoreMinimal.h"
#include "Save/LRSaveV2Types.h"

namespace LRSaveOperationQueue
{
	LOSTRUNIC_API void Enqueue(TArray<FLRQueuedSaveOperation>& queue, FLRQueuedSaveOperation&& operation);
	LOSTRUNIC_API void EnqueueFront(TArray<FLRQueuedSaveOperation>& queue, FLRQueuedSaveOperation&& operation);
	LOSTRUNIC_API bool Dequeue(TArray<FLRQueuedSaveOperation>& queue, FLRQueuedSaveOperation& outOperation);
	LOSTRUNIC_API bool HasCatalogTransaction(const FLRQueuedSaveOperation& activeOperation,
		ELRSaveOperationState operationState);
}
