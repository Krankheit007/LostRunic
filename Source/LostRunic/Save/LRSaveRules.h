#pragma once

#include "Save/LRSaveTypes.h"

/** Pure save-slot and memory transaction rules used by runtime code and automation tests. */
namespace LRSaveRules
{
	LOSTRUNIC_API FString MakeSlotName(ELRSaveSlotType slotType, int32 manualSlotIndex = INDEX_NONE);
	LOSTRUNIC_API bool IsManualSlotValid(int32 manualSlotIndex, int32 manualSlotCount);
	LOSTRUNIC_API bool IsManualSaveAllowed(ELRMemoryTransactionPhase phase);
	LOSTRUNIC_API bool CanBeginMemoryTransaction(ELRMemoryTransactionPhase phase, const FLRResumeAnchor& anchor);
	LOSTRUNIC_API bool IsMemoryEntryWorld(ELRMemoryTransactionPhase phase, FName currentMapId);
	LOSTRUNIC_API bool IsResumeWorld(ELRMemoryTransactionPhase phase, FName currentMapId, const FLRResumeAnchor& anchor);
	LOSTRUNIC_API ELRMemoryTransactionPhase ResolveAfterWrite(ELRMemoryTransactionPhase phase,
		ELRSaveWriteKind writeKind, bool bSuccess);
}
