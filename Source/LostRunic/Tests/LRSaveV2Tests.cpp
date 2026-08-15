#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Kismet/GameplayStatics.h"

#include "Save/LRSaveCatalog.h"
#include "Save/LRSaveCatalogStore.h"
#include "Save/LRSaveOperationQueue.h"
#include "Save/LRSavePayload.h"
#include "Save/LRSaveRules.h"

namespace
{
	constexpr int32 TestUserIndex = 0;

	struct FCatalogBackup
	{
		TObjectPtr<USaveGame> A;
		TObjectPtr<USaveGame> B;
		bool bHadA = false;
		bool bHadB = false;
	};

	FCatalogBackup BackupCatalogs()
	{
		FCatalogBackup backup;
		backup.bHadA = UGameplayStatics::DoesSaveGameExist(LRSaveCatalogNames::A(), TestUserIndex);
		backup.bHadB = UGameplayStatics::DoesSaveGameExist(LRSaveCatalogNames::B(), TestUserIndex);
		backup.A = backup.bHadA ? UGameplayStatics::LoadGameFromSlot(LRSaveCatalogNames::A(), TestUserIndex) : nullptr;
		backup.B = backup.bHadB ? UGameplayStatics::LoadGameFromSlot(LRSaveCatalogNames::B(), TestUserIndex) : nullptr;
		return backup;
	}

	void RestoreCatalogs(const FCatalogBackup& backup)
	{
		if (backup.bHadA && backup.A) UGameplayStatics::SaveGameToSlot(backup.A, LRSaveCatalogNames::A(), TestUserIndex);
		else UGameplayStatics::DeleteGameInSlot(LRSaveCatalogNames::A(), TestUserIndex);
		if (backup.bHadB && backup.B) UGameplayStatics::SaveGameToSlot(backup.B, LRSaveCatalogNames::B(), TestUserIndex);
		else UGameplayStatics::DeleteGameInSlot(LRSaveCatalogNames::B(), TestUserIndex);
	}

	FLRSaveSlotMetadata MakeMetadata(const int32 displayIndex, const int64 sequence)
	{
		FLRSaveSlotMetadata metadata;
		metadata.SlotId.Type = ELRSaveSlotType::Manual;
		metadata.SlotId.Guid = FGuid::NewGuid();
		metadata.DisplayIndex = displayIndex;
		metadata.SaveSequence = sequence;
		metadata.PayloadKey = FLRSaveCatalogStore::MakePayloadKey(metadata.SlotId, sequence);
		return metadata;
	}

	ULRSavePayload* MakePayload(const FLRSaveSlotMetadata& metadata)
	{
		ULRSavePayload* payload = NewObject<ULRSavePayload>();
		payload->SlotId = metadata.SlotId;
		payload->PayloadKey = metadata.PayloadKey;
		payload->SaveSequence = metadata.SaveSequence;
		payload->Data.Player.CurrentMapId = TEXT("Home");
		payload->Data.Player.ResumeAnchor.MapId = TEXT("Home");
		payload->Data.Player.ResumeAnchor.AnchorId = TEXT("Home.Start");
		return payload;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRSaveV2CatalogIdentityTest,
	"LostRunic.Save.V2.CatalogEnforcesStableIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRSaveV2CatalogIdentityTest::RunTest(const FString& parameters)
{
	ULRSaveCatalog* catalog = NewObject<ULRSaveCatalog>();
	catalog->Slots.Add(MakeMetadata(1, 1));
	catalog->Slots.Add(MakeMetadata(3, 2));
	TestEqual(TEXT("Deleted display number is reused"), catalog->FindLowestFreeDisplayIndex(20), 2);
	const FLRSaveSlotMetadata duplicate = catalog->Slots[0];
	catalog->Slots.Add(duplicate);
	FString error;
	TestFalse(TEXT("Duplicate stable slot identity is rejected"), catalog->Validate(error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRSaveV2AutoSlotTest, "LostRunic.Save.V2.AutomaticSlotIsUnique",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRSaveV2AutoSlotTest::RunTest(const FString& parameters)
{
	ULRSaveCatalog* catalog = NewObject<ULRSaveCatalog>();
	FLRSaveSlotMetadata automatic = MakeMetadata(0, 1);
	automatic.SlotId.Type = ELRSaveSlotType::Auto;
	automatic.SlotId.Guid = LRSaveV2Ids::AutoSlotGuid;
	automatic.PayloadKey = FLRSaveCatalogStore::MakePayloadKey(automatic.SlotId, 1);
	catalog->Slots.Add(automatic);
	FString error;
	TestTrue(TEXT("Fixed automatic slot is valid"), catalog->Validate(error));
	automatic.SaveSequence = 2;
	automatic.PayloadKey = FLRSaveCatalogStore::MakePayloadKey(automatic.SlotId, 2);
	catalog->Slots.Add(automatic);
	TestFalse(TEXT("Second automatic catalog entry is rejected"), catalog->Validate(error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRSaveV2PayloadIdentityTest,
	"LostRunic.Save.V2.PayloadMustMatchCatalogIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRSaveV2PayloadIdentityTest::RunTest(const FString& parameters)
{
	const FLRSaveSlotMetadata metadata = MakeMetadata(1, 7);
	ULRSavePayload* payload = MakePayload(metadata);
	ELRSaveSlotHealth health;
	FString error;
	TestTrue(TEXT("Matching payload validates"), payload->ValidatePayload(&metadata, health, error));
	payload->PayloadKey = TEXT("OtherPayload");
	TestFalse(TEXT("Payload key mismatch is rejected"), payload->ValidatePayload(&metadata, health, error));
	TestEqual(TEXT("Mismatch has deterministic health"), health, ELRSaveSlotHealth::CatalogMismatch);
	TestTrue(TEXT("Catalog mismatch may persist"), FLRSaveCatalogStore::IsDeterministicHealth(health));
	TestFalse(TEXT("Transient read failure health is not persisted"),
		FLRSaveCatalogStore::IsDeterministicHealth(ELRSaveSlotHealth::CorruptPayload));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRSaveV2QueueInvariantTest,
	"LostRunic.Save.V2.QueueIsSoleCatalogTransactionBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRSaveV2QueueInvariantTest::RunTest(const FString& parameters)
{
	TArray<FLRQueuedSaveOperation> queue;
	for (const ELRSaveOperationType type : { ELRSaveOperationType::AutoSave, ELRSaveOperationType::Delete })
	{
		FLRQueuedSaveOperation operation;
		operation.OperationId = FGuid::NewGuid();
		operation.Type = type;
		LRSaveOperationQueue::Enqueue(queue, MoveTemp(operation));
	}
	FLRQueuedSaveOperation active;
	TestTrue(TEXT("First operation dequeues"), LRSaveOperationQueue::Dequeue(queue, active));
	TestEqual(TEXT("FIFO preserves autosave first"), active.Type, ELRSaveOperationType::AutoSave);
	TestTrue(TEXT("Payload write owns catalog transaction"), LRSaveOperationQueue::HasCatalogTransaction(
		active, ELRSaveOperationState::WritingPayload));
	TestFalse(TEXT("World restore is not a catalog transaction"), LRSaveOperationQueue::HasCatalogTransaction(
		active, ELRSaveOperationState::Restoring));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRSaveV2ManualPauseRuleTest,
	"LostRunic.Save.V2.ManualSaveRequiresRealPause",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRSaveV2ManualPauseRuleTest::RunTest(const FString& parameters)
{
	TestFalse(TEXT("Unpaused gameplay rejects manual save"),
		LRSaveRules::IsManualSaveAllowed(ELRMemoryTransactionPhase::None, false));
	TestTrue(TEXT("Paused gameplay permits manual save"),
		LRSaveRules::IsManualSaveAllowed(ELRMemoryTransactionPhase::None, true));
	TestFalse(TEXT("Memory transaction rejects manual save even while paused"),
		LRSaveRules::IsManualSaveAllowed(ELRMemoryTransactionPhase::InMemory, true));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRSaveV2CrashMatrixTest,
	"LostRunic.Save.V2.CrashMatrix.PendingWriteAndDeleteRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRSaveV2CrashMatrixTest::RunTest(const FString& parameters)
{
	const FCatalogBackup backup = BackupCatalogs();
	UGameplayStatics::DeleteGameInSlot(LRSaveCatalogNames::A(), TestUserIndex);
	UGameplayStatics::DeleteGameInSlot(LRSaveCatalogNames::B(), TestUserIndex);
	const FLRSaveSlotMetadata metadata = MakeMetadata(1, FMath::RandHelper(100000) + 1);
	ULRSavePayload* payload = MakePayload(metadata);
	const FString payloadKey = metadata.PayloadKey;
	bool bSuccess = true;
	UGameplayStatics::DeleteGameInSlot(payloadKey, TestUserIndex);
	bSuccess &= UGameplayStatics::SaveGameToSlot(payload, payloadKey, TestUserIndex);

	ULRSaveCatalog* pendingWrite = NewObject<ULRSaveCatalog>();
	pendingWrite->Generation = 10;
	pendingWrite->PendingOperation.Type = ELRCatalogPendingType::Write;
	pendingWrite->PendingOperation.TargetMetadata = metadata;
	bSuccess &= UGameplayStatics::SaveGameToSlot(pendingWrite, LRSaveCatalogNames::A(), TestUserIndex);
	FLRCatalogRecoveryResult recovery;
	ULRSaveCatalog* recoveredWrite = FLRSaveCatalogStore::LoadBestCatalog(GetTransientPackage(), recovery);
	TestTrue(TEXT("PendingWrite recovery returns a catalog"), recoveredWrite != nullptr);
	TestTrue(TEXT("PendingWrite commits validated payload"), recoveredWrite && recoveredWrite->FindSlot(metadata.SlotId));
	TestFalse(TEXT("PendingWrite is cleared after recovery"), recoveredWrite && recoveredWrite->PendingOperation.IsSet());

	ULRSaveCatalog* pendingDelete = NewObject<ULRSaveCatalog>();
	pendingDelete->Generation = 20;
	pendingDelete->Slots.Add(metadata);
	pendingDelete->PendingOperation.Type = ELRCatalogPendingType::Delete;
	pendingDelete->PendingOperation.PreviousMetadata = metadata;
	pendingDelete->PendingOperation.TargetMetadata = metadata;
	bSuccess &= UGameplayStatics::SaveGameToSlot(pendingDelete, LRSaveCatalogNames::A(), TestUserIndex);
	FLRCatalogRecoveryResult deleteRecovery;
	ULRSaveCatalog* recoveredDelete = FLRSaveCatalogStore::LoadBestCatalog(GetTransientPackage(), deleteRecovery);
	TestTrue(TEXT("PendingDelete recovery returns a catalog"), recoveredDelete != nullptr);
	TestFalse(TEXT("PendingDelete removes catalog reference"), recoveredDelete && recoveredDelete->FindSlot(metadata.SlotId));
	TestFalse(TEXT("PendingDelete removes payload"), UGameplayStatics::DoesSaveGameExist(payloadKey, TestUserIndex));

	UGameplayStatics::DeleteGameInSlot(payloadKey, TestUserIndex);
	RestoreCatalogs(backup);
	return bSuccess;
}

#endif
