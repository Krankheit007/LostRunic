#include "Save/LRSaveRules.h"

#include "Save/LRSaveTypes.h"

FString LRSaveRules::MakeSlotName(const ELRSaveSlotType slotType, const int32 manualSlotIndex)
{
	return slotType == ELRSaveSlotType::Auto ? TEXT("LostRunic_Auto")
		: FString::Printf(TEXT("LostRunic_Manual_%02d"), manualSlotIndex + 1);
}

bool LRSaveRules::IsManualSlotValid(const int32 manualSlotIndex, const int32 manualSlotCount)
{
	return manualSlotIndex >= 0 && manualSlotIndex < manualSlotCount;
}

bool LRSaveRules::IsManualSaveAllowed(const ELRMemoryTransactionPhase phase)
{
	return phase == ELRMemoryTransactionPhase::None;
}

bool LRSaveRules::CanBeginMemoryTransaction(const ELRMemoryTransactionPhase phase, const FLRResumeAnchor& anchor)
{
	return phase == ELRMemoryTransactionPhase::None && anchor.IsValid();
}

bool LRSaveRules::IsMemoryEntryWorld(const ELRMemoryTransactionPhase phase, const FName currentMapId)
{
	return phase == ELRMemoryTransactionPhase::AwaitingMemoryWorld && currentMapId == LRSaveIds::MemoryMapId;
}

bool LRSaveRules::IsResumeWorld(const ELRMemoryTransactionPhase phase, const FName currentMapId, const FLRResumeAnchor& anchor)
{
	return phase == ELRMemoryTransactionPhase::AwaitingResumeWorld && anchor.IsValid() && currentMapId == anchor.MapId;
}

ELRMemoryTransactionPhase LRSaveRules::ResolveAfterWrite(const ELRMemoryTransactionPhase phase,
	const ELRSaveWriteKind writeKind, const bool bSuccess)
{
	if (!bSuccess)
	{
		return phase;
	}
	if (phase == ELRMemoryTransactionPhase::SavingEntry && writeKind == ELRSaveWriteKind::MemoryEntry)
	{
		return ELRMemoryTransactionPhase::InMemory;
	}
	if (phase == ELRMemoryTransactionPhase::SavingReturn && writeKind == ELRSaveWriteKind::MemoryReturn)
	{
		return ELRMemoryTransactionPhase::None;
	}
	return phase;
}
