#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

#include "Narrative/LRDialogueSubsystem.h"
#include "Save/LRSaveCatalog.h"
#include "Save/LRSaveCatalogStore.h"
#include "Save/LRGameStatisticsSubsystem.h"
#include "Save/LRSaveOperationQueue.h"
#include "Save/LRSavePayload.h"
#include "Save/LRSaveRules.h"
#include "Save/LRSaveSubsystem.h"
#include "HAL/FileManager.h"

namespace
{
	constexpr int32 SaveTestUserIndex = 0;

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
		backup.bHadA = UGameplayStatics::DoesSaveGameExist(LRSaveCatalogNames::A(), SaveTestUserIndex);
		backup.bHadB = UGameplayStatics::DoesSaveGameExist(LRSaveCatalogNames::B(), SaveTestUserIndex);
		backup.A = backup.bHadA ? UGameplayStatics::LoadGameFromSlot(LRSaveCatalogNames::A(), SaveTestUserIndex) : nullptr;
		backup.B = backup.bHadB ? UGameplayStatics::LoadGameFromSlot(LRSaveCatalogNames::B(), SaveTestUserIndex) : nullptr;
		return backup;
	}

	void RestoreCatalogs(const FCatalogBackup& backup)
	{
		if (backup.bHadA && backup.A) UGameplayStatics::SaveGameToSlot(backup.A, LRSaveCatalogNames::A(), SaveTestUserIndex);
		else UGameplayStatics::DeleteGameInSlot(LRSaveCatalogNames::A(), SaveTestUserIndex);
		if (backup.bHadB && backup.B) UGameplayStatics::SaveGameToSlot(backup.B, LRSaveCatalogNames::B(), SaveTestUserIndex);
		else UGameplayStatics::DeleteGameInSlot(LRSaveCatalogNames::B(), SaveTestUserIndex);
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
		payload->MetadataSnapshot = metadata;
		return payload;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRSaveManualPauseRuleTest,
	"LostRunic.Save.ManualSaveRequiresRealPause",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRSaveManualPauseRuleTest::RunTest(const FString& parameters)
{
	TestFalse(TEXT("Unpaused gameplay rejects manual save"),
		LRSaveRules::IsManualSaveAllowed(ELRMemoryTransactionPhase::None, false));
	TestTrue(TEXT("Paused gameplay permits manual save"),
		LRSaveRules::IsManualSaveAllowed(ELRMemoryTransactionPhase::None, true));
	TestFalse(TEXT("Memory transaction rejects manual save even while paused"),
		LRSaveRules::IsManualSaveAllowed(ELRMemoryTransactionPhase::InMemory, true));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRSaveMemoryPurposeSequenceTest,
	"LostRunic.Save.MemoryCriticalOperationsPreservePurposeOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRSaveMemoryPurposeSequenceTest::RunTest(const FString& parameters)
{
	TArray<FLRQueuedSaveOperation> queue;
	for (const ELRSaveMemoryPurpose purpose : { ELRSaveMemoryPurpose::Entry,
		ELRSaveMemoryPurpose::Event, ELRSaveMemoryPurpose::Return })
	{
		FLRQueuedSaveOperation operation;
		operation.OperationId = FGuid::NewGuid();
		operation.Type = ELRSaveOperationType::CriticalSave;
		operation.MemoryPurpose = purpose;
		LRSaveOperationQueue::Enqueue(queue, MoveTemp(operation));
	}

	for (const ELRSaveMemoryPurpose expected : { ELRSaveMemoryPurpose::Entry,
		ELRSaveMemoryPurpose::Event, ELRSaveMemoryPurpose::Return })
	{
		FLRQueuedSaveOperation operation;
		TestTrue(TEXT("Memory critical operation dequeues"), LRSaveOperationQueue::Dequeue(queue, operation));
		TestEqual(TEXT("Memory purpose remains FIFO"), operation.MemoryPurpose, expected);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRSaveImmutableOperationSnapshotTest,
	"LostRunic.Save.OperationSnapshotIsImmutable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRSaveImmutableOperationSnapshotTest::RunTest(const FString& parameters)
{
	FLRSaveDataV2 captured;
	captured.Statistics.DeathCount = 1;
	captured.Story.MemoryEventIds.Add(TEXT("Memory.Entry"));

	FLRQueuedSaveOperation operation;
	operation.OperationId = FGuid::NewGuid();
	operation.Type = ELRSaveOperationType::CriticalSave;
	operation.CapturedData = captured;

	captured.Statistics.DeathCount = 2;
	captured.Story.MemoryEventIds.Add(TEXT("Memory.Late"));

	TestEqual(TEXT("Operation owns the captured death count"), operation.CapturedData.Statistics.DeathCount, 1);
	TestTrue(TEXT("Operation owns the captured memory event"),
		operation.CapturedData.Story.MemoryEventIds.Contains(TEXT("Memory.Entry")));
	TestFalse(TEXT("Later mutable event is not shared"),
		operation.CapturedData.Story.MemoryEventIds.Contains(TEXT("Memory.Late")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRSaveMemoryStateOwnerRoundTripTest,
	"LostRunic.Save.V2.MemoryStateOwnersRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRSaveMemoryStateOwnerRoundTripTest::RunTest(const FString& parameters)
{
	UGameInstance* gameInstance = NewObject<UGameInstance>();
	ULRGameStatisticsSubsystem* statistics = NewObject<ULRGameStatisticsSubsystem>(gameInstance);
	statistics->RecordDeath();
	FLRSaveStatisticsChunk savedStatistics;
	statistics->Capture(savedStatistics);

	ULRDialogueSubsystem* dialogue = NewObject<ULRDialogueSubsystem>(gameInstance);
	TestTrue(TEXT("Dialogue owner records a Memory event"), dialogue->RecordMemoryEvent(TEXT("Memory.Entry")));
	FLRSaveStoryChunk savedStory;
	dialogue->CaptureStorySaveState(savedStory);

	ULRGameStatisticsSubsystem* restoredStatistics = NewObject<ULRGameStatisticsSubsystem>(gameInstance);
	restoredStatistics->Restore(savedStatistics);
	FLRSaveStatisticsChunk restoredStatisticsData;
	restoredStatistics->Capture(restoredStatisticsData);
	TestEqual(TEXT("DeathCount survives V2 statistics capture and restore"),
		restoredStatisticsData.DeathCount, savedStatistics.DeathCount);

	ULRDialogueSubsystem* restoredDialogue = NewObject<ULRDialogueSubsystem>(gameInstance);
	restoredDialogue->RestoreStorySaveState(savedStory);
	FLRSaveStoryChunk restoredStory;
	restoredDialogue->CaptureStorySaveState(restoredStory);
	TestTrue(TEXT("MemoryEventIds survive V2 story capture and restore"),
		restoredStory.MemoryEventIds.Contains(TEXT("Memory.Entry")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRSaveProtectedAutomaticOverwriteTest,
	"LostRunic.Save.V2.OverwriteAutomaticSlotIsAlwaysProtected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRSaveProtectedAutomaticOverwriteTest::RunTest(const FString& parameters)
{
	FLRSaveSlotId autoSlot;
	autoSlot.Type = ELRSaveSlotType::Auto;
	autoSlot.Guid = LRSaveV2Ids::AutoSlotGuid;
	TestTrue(TEXT("Automatic overwrite is protected before eligibility checks"),
		LRSaveRules::IsProtectedOverwrite(autoSlot));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRSaveRepairQueuePriorityTest,
	"LostRunic.Save.V2.RepairHealthHasQueueHeadPriority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRSaveRepairQueuePriorityTest::RunTest(const FString& parameters)
{
	TArray<FLRQueuedSaveOperation> queue;
	FLRQueuedSaveOperation normal;
	normal.OperationId = FGuid::NewGuid();
	normal.Type = ELRSaveOperationType::AutoSave;
	LRSaveOperationQueue::Enqueue(queue, MoveTemp(normal));

	FLRQueuedSaveOperation repair;
	repair.OperationId = FGuid::NewGuid();
	repair.Type = ELRSaveOperationType::RepairHealth;
	LRSaveOperationQueue::EnqueueFront(queue, MoveTemp(repair));

	FLRQueuedSaveOperation active;
	TestTrue(TEXT("Repair operation dequeues first"), LRSaveOperationQueue::Dequeue(queue, active));
	TestEqual(TEXT("Repair operation owns queue head"), active.Type, ELRSaveOperationType::RepairHealth);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRSaveCatalogBootstrapReadOnlyTest,
	"LostRunic.Save.V2.LoadBestCatalogIsReadOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRSaveCatalogBootstrapReadOnlyTest::RunTest(const FString& parameters)
{
	const FCatalogBackup backup = BackupCatalogs();
	ULRSaveCatalog* pending = NewObject<ULRSaveCatalog>();
	pending->Generation = 100;
	pending->PendingOperation.Type = ELRCatalogPendingType::Delete;
	pending->PendingOperation.TargetMetadata = MakeMetadata(1, 1);
	TestTrue(TEXT("Test catalog is written"),
		UGameplayStatics::SaveGameToSlot(pending, LRSaveCatalogNames::A(), SaveTestUserIndex));

	FLRCatalogRecoveryResult result;
	ULRSaveCatalog* loaded = FLRSaveCatalogStore::LoadBestCatalog(GetTransientPackage(), result);
	TestNotNull(TEXT("Pending catalog is selected"), loaded);
	TestTrue(TEXT("Bootstrap leaves pending transaction for RepairHealth"),
		loaded && loaded->PendingOperation.IsSet());

	ULRSaveCatalog* diskCatalog = Cast<ULRSaveCatalog>(
		UGameplayStatics::LoadGameFromSlot(LRSaveCatalogNames::A(), SaveTestUserIndex));
	TestTrue(TEXT("Bootstrap does not change disk generation"), diskCatalog && diskCatalog->Generation == 100);
	TestTrue(TEXT("Bootstrap does not change disk pending state"), diskCatalog && diskCatalog->PendingOperation.IsSet());
	RestoreCatalogs(backup);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRSaveCatalogRecoveryDispatchTest,
	"LostRunic.Save.V2.RepairHealthOwnsPendingTransactionRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRSaveCatalogRecoveryDispatchTest::RunTest(const FString& parameters)
{
	const FCatalogBackup backup = BackupCatalogs();
	ULRSaveCatalog* pending = NewObject<ULRSaveCatalog>();
	pending->Generation = 101;
	const FLRSaveSlotMetadata metadata = MakeMetadata(1, 2);
	pending->PendingOperation.Type = ELRCatalogPendingType::Write;
	pending->PendingOperation.TargetMetadata = metadata;
	TestTrue(TEXT("Pending catalog is written"),
		UGameplayStatics::SaveGameToSlot(pending, LRSaveCatalogNames::A(), SaveTestUserIndex));
	ULRSavePayload* payload = MakePayload(metadata);
	TestTrue(TEXT("Pending payload is written"),
		UGameplayStatics::SaveGameToSlot(payload, metadata.PayloadKey, SaveTestUserIndex));

	FLRCatalogRecoveryResult bootstrapResult;
	ULRSaveCatalog* bootstrapped = FLRSaveCatalogStore::LoadBestCatalog(GetTransientPackage(), bootstrapResult);
	TestTrue(TEXT("Bootstrap keeps pending write"), bootstrapped && bootstrapped->PendingOperation.IsSet());
	FString error;
	TestTrue(TEXT("Repair operation can recover pending write"),
		bootstrapped && FLRSaveCatalogStore::RecoverPendingOperation(*bootstrapped, error));
	TestFalse(TEXT("Repair clears pending write"), bootstrapped && bootstrapped->PendingOperation.IsSet());
	TestNotNull(TEXT("Repair commits the validated payload metadata"),
		bootstrapped ? bootstrapped->FindSlot(metadata.SlotId) : nullptr);

	UGameplayStatics::DeleteGameInSlot(metadata.PayloadKey, SaveTestUserIndex);
	RestoreCatalogs(backup);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRSaveContinueCandidateRuleTest,
	"LostRunic.Save.Catalog.ContinueUsesHealthyCandidateRule",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRSaveContinueCandidateRuleTest::RunTest(const FString& parameters)
{
	FLRSaveSlotMetadata newestHealthy = MakeMetadata(2, 3);
	newestHealthy.SavedAtUtc = FDateTime(2026, 8, 16, 12, 0, 0, 0);
	FLRSaveSlotMetadata olderHealthy = MakeMetadata(1, 2);
	olderHealthy.SavedAtUtc = FDateTime(2026, 8, 16, 11, 0, 0, 0);
	FLRSaveSlotMetadata newestCorrupt = MakeMetadata(3, 4);
	newestCorrupt.SavedAtUtc = FDateTime(2026, 8, 16, 13, 0, 0, 0);
	newestCorrupt.Health = ELRSaveSlotHealth::CorruptPayload;

	FLRSaveSlotId candidate;
	TestTrue(TEXT("Continue resolves a healthy candidate"),
		LRSaveRules::ResolveContinueCandidate({ newestCorrupt, olderHealthy, newestHealthy }, candidate));
	TestEqual(TEXT("Continue uses the newest healthy metadata"), candidate, newestHealthy.SlotId);
	TestTrue(TEXT("CanContinue is true when a healthy candidate exists"),
		LRSaveRules::CanContinue({ newestCorrupt, olderHealthy, newestHealthy }));
	TestFalse(TEXT("CanContinue is false when every metadata entry is unhealthy"),
		LRSaveRules::CanContinue({ newestCorrupt }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRSaveCatalogNotReadyGateTest,
	"LostRunic.Save.Catalog.NotReadyQueriesAndRequestsAreGated",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRSaveCatalogNotReadyGateTest::RunTest(const FString& parameters)
{
	UGameInstance* gameInstance = NewObject<UGameInstance>();
	ULRSaveSubsystem* subsystem = NewObject<ULRSaveSubsystem>(gameInstance);
	TestEqual(TEXT("Fresh subsystem starts Initializing"), subsystem->GetCatalogState(), ELRSaveCatalogState::Initializing);
	TestTrue(TEXT("Non-ready slot query is empty"), subsystem->GetSaveSlots().IsEmpty());
	TestFalse(TEXT("Non-ready catalog has no formal entry"), subsystem->HasAnyCatalogEntry());
	TestFalse(TEXT("Non-ready catalog cannot Continue"), subsystem->CanContinue());
	TestEqual(TEXT("Create is rejected busy before catalog Ready"),
		subsystem->RequestCreateManualSave(TEXT("Test")).Code, ELRSaveResultCode::RejectedBusy);
	TestEqual(TEXT("Continue is rejected busy before catalog Ready"),
		subsystem->RequestContinue().Code, ELRSaveResultCode::RejectedBusy);
	TestEqual(TEXT("New Game is rejected busy before catalog Ready"),
		subsystem->RequestNewGame().Code, ELRSaveResultCode::RejectedBusy);
	FLRSaveSlotId slotId;
	slotId.Type = ELRSaveSlotType::Manual;
	slotId.Guid = FGuid::NewGuid();
	TestEqual(TEXT("Load is rejected busy before catalog Ready"),
		subsystem->RequestLoadSave(slotId).Code, ELRSaveResultCode::RejectedBusy);
	TestEqual(TEXT("Delete is rejected busy before catalog Ready"),
		subsystem->RequestDeleteSave(slotId).Code, ELRSaveResultCode::RejectedBusy);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRSaveCatalogV1FixtureLoadTest,
	"LostRunic.Save.Catalog.LoadsFrozenV1Fixture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRSaveCatalogV1FixtureLoadTest::RunTest(const FString& parameters)
{
	const FString fixturePath = FPaths::ProjectDir() / TEXT("Source/LostRunic/Tests/Fixtures/CatalogV1_NoCollectedCount.bin");
	TArray<uint8> bytes;
	TestTrue(TEXT("Frozen V1 fixture exists"), FFileHelper::LoadFileToArray(bytes, *fixturePath));
	if (bytes.IsEmpty())
	{
		return false;
	}

	const FString temporarySlot = TEXT("LostRunic_Test_CatalogV1_Load");
	const FString saveDirectory = FPaths::ProjectSavedDir() / TEXT("SaveGames");
	IFileManager::Get().MakeDirectory(*saveDirectory, true);
	const FString savedFile = saveDirectory / (temporarySlot + TEXT(".sav"));
	TestTrue(TEXT("Frozen V1 fixture is copied to a SaveGame slot"), FFileHelper::SaveArrayToFile(bytes, *savedFile));
	ULRSaveCatalog* catalog = Cast<ULRSaveCatalog>(UGameplayStatics::LoadGameFromSlot(temporarySlot, SaveTestUserIndex));
	TestNotNull(TEXT("Frozen V1 catalog loads"), catalog);
	if (!catalog)
	{
		UGameplayStatics::DeleteGameInSlot(temporarySlot, SaveTestUserIndex);
		return false;
	}

	TestEqual(TEXT("Frozen V1 catalog keeps generation"), catalog->Generation, int64(7));
	TestEqual(TEXT("Frozen V1 catalog has one slot"), catalog->Slots.Num(), 1);
	if (catalog->Slots.Num() == 1)
	{
		const FLRSaveSlotMetadata& metadata = catalog->Slots[0];
		const FGuid expectedGuid(0xA1B2C3D4, 0x01020304, 0x55667788, 0x99AABBCC);
		TestTrue(TEXT("Frozen V1 slot keeps identity"), metadata.SlotId.Guid == expectedGuid);
		TestEqual(TEXT("Frozen V1 slot keeps display index"), metadata.DisplayIndex, 2);
		TestEqual(TEXT("Frozen V1 slot keeps payload key"), metadata.PayloadKey, FString(TEXT("LostRunic_V1_Fixture_Payload")));
		TestEqual(TEXT("Frozen V1 slot keeps map"), metadata.MapId, FName(TEXT("Home")));
		TestEqual(TEXT("Frozen V1 slot keeps UTC timestamp"), metadata.SavedAtUtc, FDateTime(2024, 1, 2, 3, 4, 5, 6));
		TestEqual(TEXT("Frozen V1 slot keeps play time"), metadata.PlayTimeSeconds, 3723.5);
		TestEqual(TEXT("Frozen V1 slot keeps save sequence"), metadata.SaveSequence, int64(42));
		TestEqual(TEXT("Frozen V1 slot keeps health"), metadata.Health, ELRSaveSlotHealth::Healthy);
		TestEqual(TEXT("New collected count defaults for V1 data"), metadata.CollectedCount, 0);
	}

	UGameplayStatics::DeleteGameInSlot(temporarySlot, SaveTestUserIndex);
	return true;
}

#endif
