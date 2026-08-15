/**
 * @file LRSaveTests.cpp
 * @brief 提供 LostRunic Runtime 自动化测试，覆盖调优边界、状态矩阵、交互筛选、物品双入口、守卫警戒、叙事分支和存档事务顺序。仅在 WITH_DEV_AUTOMATION_TESTS 下编译。
 *
 * 关联文件：Tests 目录内调用该公共契约的实现文件；所属领域：Tests。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Save/LRSaveGame.h"
#include "Save/LRSaveRequestQueue.h"
#include "Save/LRSaveRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRSaveMigrationTest, "LostRunic.Save.VersionZeroMigratesResumeAnchor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRSaveMigrationTest::RunTest(const FString& parameters)
{
	ULRSaveGame* save = NewObject<ULRSaveGame>();
	save->SaveVersion = 0;
	save->LegacyMapId = TEXT("Home");
	save->LegacyLocation = FVector(10.0f, 20.0f, 30.0f);
	save->LegacyRotation = FRotator(0.0f, 90.0f, 0.0f);
	FString error;
	TestTrue(TEXT("Version zero migrates"), save->MigrateToLatest(error));
	TestEqual(TEXT("Migration stamps latest version"), save->SaveVersion, ULRSaveGame::LatestVersion);
	TestEqual(TEXT("Migration preserves stable map ID"), save->ResumeAnchor.MapId, FName(TEXT("Home")));
	TestEqual(TEXT("Migration preserves anchor location"), save->ResumeAnchor.Location, FVector(10.0f, 20.0f, 30.0f));
	TestEqual(TEXT("Migration preserves anchor rotation"), save->ResumeAnchor.Rotation, FRotator(0.0f, 90.0f, 0.0f));
	TestTrue(TEXT("Migration produces a valid resume anchor"), save->ResumeAnchor.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRSaveSlotRulesTest, "LostRunic.Save.SlotRulesAndMemoryManualBlock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRSaveSlotRulesTest::RunTest(const FString& parameters)
{
	TestEqual(TEXT("One automatic slot has a stable name"), LRSaveRules::MakeSlotName(ELRSaveSlotType::Auto), FString(TEXT("LostRunic_Auto")));
	TestEqual(TEXT("First manual slot has a stable name"), LRSaveRules::MakeSlotName(ELRSaveSlotType::Manual, 0), FString(TEXT("LostRunic_Manual_01")));
	TestTrue(TEXT("Tenth manual slot is valid"), LRSaveRules::IsManualSlotValid(9, 10));
	TestFalse(TEXT("Eleventh manual slot is rejected"), LRSaveRules::IsManualSlotValid(10, 10));
	TestTrue(TEXT("Manual saves are legal while paused outside Memory"),
		LRSaveRules::IsManualSaveAllowed(ELRMemoryTransactionPhase::None, true));
	TestFalse(TEXT("Manual saves are blocked while unpaused"),
		LRSaveRules::IsManualSaveAllowed(ELRMemoryTransactionPhase::None, false));
	TestFalse(TEXT("Manual saves are blocked during Memory"),
		LRSaveRules::IsManualSaveAllowed(ELRMemoryTransactionPhase::InMemory, true));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRSaveMemorySequenceTest, "LostRunic.Save.MemoryCriticalSequencePreservesAnchor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRSaveMemorySequenceTest::RunTest(const FString& parameters)
{
	FLRResumeAnchor anchor;
	anchor.MapId = TEXT("Home");
	anchor.AnchorId = TEXT("Home.Hall");
	TestTrue(TEXT("Memory begins only from a stable resume anchor"),
		LRSaveRules::CanBeginMemoryTransaction(ELRMemoryTransactionPhase::None, anchor));
	TestTrue(TEXT("Memory world accepts entry critical save A"),
		LRSaveRules::IsMemoryEntryWorld(ELRMemoryTransactionPhase::AwaitingMemoryWorld, LRSaveIds::MemoryMapId));
	TestEqual(TEXT("Critical A enters Memory"), LRSaveRules::ResolveAfterWrite(ELRMemoryTransactionPhase::SavingEntry,
		ELRSaveWriteKind::MemoryEntry, true), ELRMemoryTransactionPhase::InMemory);
	TestTrue(TEXT("Original Home anchor remains valid in Memory"), anchor.IsValid());
	TestTrue(TEXT("Home world accepts return critical save B"),
		LRSaveRules::IsResumeWorld(ELRMemoryTransactionPhase::AwaitingResumeWorld, anchor.MapId, anchor));
	TestEqual(TEXT("Critical B closes the transaction"), LRSaveRules::ResolveAfterWrite(ELRMemoryTransactionPhase::SavingReturn,
		ELRSaveWriteKind::MemoryReturn, true), ELRMemoryTransactionPhase::None);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRSaveQueueOrderTest, "LostRunic.Save.CriticalRequestsRemainFifo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRSaveQueueOrderTest::RunTest(const FString& parameters)
{
	TArray<FLRQueuedSaveRequest> queue;
	for (const FName reason : { FName(TEXT("Memory.Entry")), FName(TEXT("Memory.Doll")), FName(TEXT("Memory.Return")) })
	{
		FLRQueuedSaveRequest request;
		request.ReasonId = reason;
		LRSaveRequestQueue::Enqueue(queue, MoveTemp(request));
	}
	for (const FName expected : { FName(TEXT("Memory.Entry")), FName(TEXT("Memory.Doll")), FName(TEXT("Memory.Return")) })
	{
		FLRQueuedSaveRequest request;
		TestTrue(TEXT("FIFO request dequeues"), LRSaveRequestQueue::Dequeue(queue, request));
		TestEqual(TEXT("FIFO preserves critical request order"), request.ReasonId, expected);
	}
	FLRQueuedSaveRequest emptyRequest;
	TestFalse(TEXT("Queue is empty after all requests"), LRSaveRequestQueue::Dequeue(queue, emptyRequest));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRSaveSnapshotTest, "LostRunic.Save.SnapshotsDoNotShareMutableProgress",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRSaveSnapshotTest::RunTest(const FString& parameters)
{
	ULRSaveGame* workingSave = NewObject<ULRSaveGame>();
	workingSave->Narrative.DeathCount = 1;
	workingSave->Narrative.MemoryEventIds.Add(TEXT("Memory.Doll"));
	ULRSaveGame* snapshot = DuplicateObject<ULRSaveGame>(workingSave, GetTransientPackage());
	workingSave->Narrative.DeathCount = 2;
	workingSave->Narrative.MemoryEventIds.Add(TEXT("Memory.NPC"));
	TestEqual(TEXT("Snapshot keeps original death count"), snapshot->Narrative.DeathCount, 1);
	TestTrue(TEXT("Snapshot keeps committed event"), snapshot->Narrative.MemoryEventIds.Contains(TEXT("Memory.Doll")));
	TestFalse(TEXT("Snapshot excludes later mutable event"), snapshot->Narrative.MemoryEventIds.Contains(TEXT("Memory.NPC")));
	return true;
}

#endif
