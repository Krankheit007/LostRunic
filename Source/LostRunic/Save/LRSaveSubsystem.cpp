/**
 * @file LRSaveSubsystem.cpp
 * @brief 管理槽位元数据、快照构建、普通自动存档防抖、失败重试、异步 FIFO 队列、继续游戏选择及 Memory A/B 事务。
 *
 * 关联文件：LRSaveSubsystem.h；所属领域：Save。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Save/LRSaveSubsystem.h"

#include "Core/LRLog.h"
#include "Data/LRGameTuningSet.h"
#include "Data/LRSaveTuning.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "Narrative/LRDialogueSubsystem.h"
#include "Save/LRSaveGame.h"
#include "Save/LRSaveCatalog.h"
#include "Save/LRSaveCatalogStore.h"
#include "Save/LRSaveProvider.h"
#include "Save/LRSaveRules.h"
#include "TimerManager.h"

/**
 * @brief 初始化子系统拥有的长期状态与事件绑定。
 * @param collection 调用方提供的 `collection`，只在本次操作范围内使用。
 */
void ULRSaveSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
	Super::Initialize(collection);
	collection.InitializeDependency<ULRGameInstanceSubsystem>();
	collection.InitializeDependency<ULRDialogueSubsystem>();
	const ULRGameInstanceSubsystem* dataSubsystem = GetGameInstance()->GetSubsystem<ULRGameInstanceSubsystem>();
	Tuning = dataSubsystem && dataSubsystem->GetTuningSet() ? dataSubsystem->GetTuningSet()->Save : nullptr;
	WorkingSave = NewObject<ULRSaveGame>(this);
	FLRCatalogRecoveryResult recovery;
	SaveCatalog = FLRSaveCatalogStore::LoadBestCatalog(this, recovery);
	LRSaveProviders::CreateRequired(SaveProviders);
	if (!recovery.Diagnostic.IsEmpty())
	{
		UE_LOG(LogLostRunicSave, Log, TEXT("V2 catalog startup: %s"), *recovery.Diagnostic);
	}
	if (ULRDialogueSubsystem* dialogueSubsystem = GetGameInstance()->GetSubsystem<ULRDialogueSubsystem>())
	{
		dialogueSubsystem->OnEventCommitted.AddDynamic(this, &ULRSaveSubsystem::HandleNarrativeEventCommitted);
	}
}

/**
 * @brief 释放子系统事件绑定和运行时缓存。
 */
void ULRSaveSubsystem::Deinitialize()
{
	if (ULRDialogueSubsystem* dialogueSubsystem = GetGameInstance()->GetSubsystem<ULRDialogueSubsystem>())
	{
		dialogueSubsystem->OnEventCommitted.RemoveDynamic(this, &ULRSaveSubsystem::HandleNarrativeEventCommitted);
	}
	if (UWorld* world = GetCurrentWorld())
	{
		world->GetTimerManager().ClearTimer(AutoSaveDebounceTimer);
		world->GetTimerManager().ClearTimer(RetryTimer);
	}
	RequestQueue.Reset();
	V2OperationQueue.Reset();
	ActiveV2Operation = FLRQueuedSaveOperation();
	LoadedV2Payload = nullptr;
	SaveCatalog = nullptr;
	SaveProviders.Reset();
	V2OperationState = ELRSaveOperationState::Idle;
	ActiveRequest = FLRQueuedSaveRequest();
	WorkingSave = nullptr;
	Tuning = nullptr;
	bAwaitingLoadedResume = false;
	Super::Deinitialize();
}

/**
 * @brief 更新 Resume Anchor，并在需要时同步组件状态或广播变化事件。
 * @param anchor 调用方提供的 `anchor`，只在本次操作范围内使用。
 */
void ULRSaveSubsystem::SetResumeAnchor(const FLRResumeAnchor& anchor)
{
	if (!anchor.IsValid())
	{
		UE_LOG(LogLostRunicSave, Warning, TEXT("SaveSubsystem rejected invalid resume anchor map=%s anchor=%s."),
			*anchor.MapId.ToString(), *anchor.AnchorId.ToString());
		return;
	}
	WorkingSave->ResumeAnchor = anchor;
}

/**
 * @brief 查询 Resume Anchor；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FLRResumeAnchor ULRSaveSubsystem::GetResumeAnchor() const
{
	return WorkingSave ? WorkingSave->ResumeAnchor : FLRResumeAnchor();
}

/**
 * @brief 请求普通自动存档；按 Save 调优资产执行防抖，不合并关键 Memory 事务。
 * @param reasonId 稳定标识 `reasonId`；用于内容查询和存档，不依赖显示名或数组序号。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
ELRSaveRequestResult ULRSaveSubsystem::RequestAutoSave(const FName reasonId)
{
	if (!WorkingSave)
	{
		return ELRSaveRequestResult::MissingOrCorrupt;
	}
	PendingAutoSaveReason = reasonId.IsNone() ? LRSaveIds::AutoSlotReason : reasonId;
	UWorld* world = GetCurrentWorld();
	if (!world || GetEffectiveTuning().AutoSaveDebounceSeconds <= 0.0f)
	{
		QueuePendingAutoSave();
		return ELRSaveRequestResult::Queued;
	}
	world->GetTimerManager().SetTimer(AutoSaveDebounceTimer, this, &ULRSaveSubsystem::QueuePendingAutoSave,
		GetEffectiveTuning().AutoSaveDebounceSeconds, false);
	return ELRSaveRequestResult::Scheduled;
}

/**
 * @brief 请求写入指定手动槽；校验槽位范围，并在 Memory 事务期间明确拒绝。
 * @param manualSlotIndex 本次操作使用的计数、增量或索引 `manualSlotIndex`；由函数校验合法范围。
 * @param reasonId 稳定标识 `reasonId`；用于内容查询和存档，不依赖显示名或数组序号。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
ELRSaveRequestResult ULRSaveSubsystem::RequestManualSave(const int32 manualSlotIndex, const FName reasonId)
{
	if (!IsManualSaveAllowed())
	{
		return ELRSaveRequestResult::RejectedMemoryManual;
	}
	if (!LRSaveRules::IsManualSlotValid(manualSlotIndex, GetManualSlotCount()))
	{
		return ELRSaveRequestResult::RejectedInvalidSlot;
	}
	return QueueWrite(LRSaveRules::MakeSlotName(ELRSaveSlotType::Manual, manualSlotIndex), reasonId, ELRSaveWriteKind::Manual);
}

/**
 * @brief 创建不可变快照并将关键写入直接加入 FIFO，不参与普通自动存档防抖。
 * @param reasonId 稳定标识 `reasonId`；用于内容查询和存档，不依赖显示名或数组序号。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
ELRSaveRequestResult ULRSaveSubsystem::RequestCriticalSave(const FName reasonId)
{
	return QueueWrite(LRSaveRules::MakeSlotName(ELRSaveSlotType::Auto), reasonId, ELRSaveWriteKind::Critical);
}

/**
 * @brief 判断 Is Manual Save Allowed 对应条件；不产生玩法副作用。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRSaveSubsystem::IsManualSaveAllowed() const
{
	const UWorld* world = GetCurrentWorld();
	return LRSaveRules::IsManualSaveAllowed(MemoryPhase, world && world->IsPaused());
}

/**
 * @brief 处理 Handle Narrative Event Committed 事件，将引擎回调转换为对应领域状态更新。
 * @param eventId 剧情事件的稳定 FName ID，用于一次性判定和存档。
 * @param savePolicy 本次操作使用的 `savePolicy` 枚举或模式值。
 */
void ULRSaveSubsystem::HandleNarrativeEventCommitted(const FName eventId, const ELRSavePolicy savePolicy)
{
	if (savePolicy == ELRSavePolicy::AutoOnComplete)
	{
		RequestAutoSave(eventId);
	}
	else if (savePolicy == ELRSavePolicy::Critical)
	{
		RequestCriticalSave(eventId);
	}
}

/**
 * @brief 查询 Effective Tuning；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
const ULRSaveTuning& ULRSaveSubsystem::GetEffectiveTuning() const
{
	return Tuning ? *Tuning : *GetDefault<ULRSaveTuning>();
}

/**
 * @brief 查询 Manual Slot Count；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
int32 ULRSaveSubsystem::GetManualSlotCount() const
{
	return GetEffectiveTuning().MaxManualSaveSlots;
}

/**
 * @brief 查询 Current World；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
UWorld* ULRSaveSubsystem::GetCurrentWorld() const
{
	return GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
}
