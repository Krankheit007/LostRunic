/**
 * @file LRSaveSubsystemQueue.cpp
 * @brief 实现一个自动槽、十个手动槽、版本迁移、不可变快照、FIFO 异步写入，以及死亡进入 Memory 和返回恢复锚点的 A/B 关键事务。
 *
 * 关联文件：Save 目录内调用该公共契约的实现文件；所属领域：Save。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Save/LRSaveSubsystem.h"

#include "Core/LRGameplayTags.h"
#include "Core/LRLog.h"
#include "Engine/World.h"
#include "Framework/LRCharacter.h"
#include "Items/LRInventoryComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Narrative/LRDialogueSubsystem.h"
#include "Save/LRSaveGame.h"
#include "Save/LRSaveRequestQueue.h"
#include "Save/LRSaveRules.h"
#include "State/LRStateComponent.h"
#include "TimerManager.h"

/**
 * @brief 从角色组件捕获位置、状态、库存和剧情进度，更新内存中的存档模型。
 */
void ULRSaveSubsystem::CaptureRuntimeState()
{
	if (!WorkingSave)
	{
		return;
	}
	const ALRCharacter* character = Cast<ALRCharacter>(UGameplayStatics::GetPlayerCharacter(GetCurrentWorld(), 0));
	if (character)
	{
		character->GetInventoryComponent()->CaptureSaveState(WorkingSave->Inventory);
	}
	if (const ULRDialogueSubsystem* dialogueSubsystem = GetGameInstance()->GetSubsystem<ULRDialogueSubsystem>())
	{
		WorkingSave->Narrative.CompletedEventIds = dialogueSubsystem->GetCompletedEvents();
	}
}

/**
 * @brief 把 Apply Runtime State 数据应用到运行时对象，并显式处理缺失依赖。
 * @param character 参与本次操作的运行时对象 `character`；函数会检查空值和所需接口。
 */
void ULRSaveSubsystem::ApplyRuntimeState(ALRCharacter* character)
{
	if (!WorkingSave)
	{
		return;
	}
	if (character)
	{
		character->GetInventoryComponent()->RestoreSaveState(WorkingSave->Inventory);
		if (GetCurrentMapId() == WorkingSave->ResumeAnchor.MapId)
		{
			character->SetActorLocationAndRotation(WorkingSave->ResumeAnchor.Location,
				WorkingSave->ResumeAnchor.Rotation, false, nullptr, ETeleportType::TeleportPhysics);
		}
		if (ULRStateComponent* state = character->GetStateComponent(); state
			&& state->GetCurrentMode() == ELRPerceptionMode::Memory)
		{
			FLRStateChangeRequest request;
			request.TargetMode = ELRPerceptionMode::Normal;
			request.RequestType = ELRStateRequestType::Narrative;
			request.Source = LRGameplayTags::StateSourceNarrative;
			state->RequestStateChange(request);
		}
	}
	if (ULRDialogueSubsystem* dialogueSubsystem = GetGameInstance()->GetSubsystem<ULRDialogueSubsystem>())
	{
		dialogueSubsystem->RestoreCompletedEvents(WorkingSave->Narrative.CompletedEventIds);
	}
}

/**
 * @brief 根据当前领域状态构建 Create Snapshot 所需的数据，不把临时对象作为长期存档标识。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
ULRSaveGame* ULRSaveSubsystem::CreateSnapshot() const
{
	return WorkingSave ? DuplicateObject<ULRSaveGame>(WorkingSave, const_cast<ULRSaveSubsystem*>(this)) : nullptr;
}

/**
 * @brief 按既定顺序将 Queue Write 请求加入队列，保留调用时快照。
 * @param slotName 实际磁盘槽名称，由自动槽或手动槽规则生成。
 * @param reasonId 稳定标识 `reasonId`；用于内容查询和存档，不依赖显示名或数组序号。
 * @param writeKind 本次操作使用的 `writeKind` 枚举或模式值。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
ELRSaveRequestResult ULRSaveSubsystem::QueueWrite(const FString& slotName, const FName reasonId,
	const ELRSaveWriteKind writeKind)
{
	if (!WorkingSave)
	{
		return ELRSaveRequestResult::MissingOrCorrupt;
	}
	CaptureRuntimeState();
	WorkingSave->LastSavedUtc = FDateTime::UtcNow();
	FLRQueuedSaveRequest request;
	request.Snapshot = CreateSnapshot();
	request.SlotName = slotName;
	request.ReasonId = reasonId.IsNone() ? LRSaveIds::AutoSlotReason : reasonId;
	request.Kind = writeKind;
	LRSaveRequestQueue::Enqueue(RequestQueue, MoveTemp(request));
	OnSaveWriteQueued.Broadcast(RequestQueue.Last().ReasonId, writeKind);
	UE_LOG(LogLostRunicSave, Log, TEXT("Save queued slot=%s reason=%s kind=%d pending=%d"), *slotName,
		*RequestQueue.Last().ReasonId.ToString(), static_cast<int32>(writeKind), RequestQueue.Num());
	StartNextWrite();
	return ELRSaveRequestResult::Queued;
}

/**
 * @brief 按既定顺序将 Queue Pending Auto Save 请求加入队列，保留调用时快照。
 */
void ULRSaveSubsystem::QueuePendingAutoSave()
{
	const FName reasonId = PendingAutoSaveReason.IsNone() ? LRSaveIds::AutoSlotReason : PendingAutoSaveReason;
	PendingAutoSaveReason = NAME_None;
	QueueWrite(LRSaveRules::MakeSlotName(ELRSaveSlotType::Auto), reasonId, ELRSaveWriteKind::Auto);
}

/**
 * @brief 开始 Start Next Write 流程，建立本次操作拥有的状态、委托或计时器。
 */
void ULRSaveSubsystem::StartNextWrite()
{
	if (bWriteInProgress || RequestQueue.IsEmpty())
	{
		return;
	}
	LRSaveRequestQueue::Dequeue(RequestQueue, ActiveRequest);
	bWriteInProgress = true;
	StartActiveWrite();
}

/**
 * @brief 启动 FIFO 队首快照的异步写盘，并保留重试次数和请求种类。
 */
void ULRSaveSubsystem::StartActiveWrite()
{
	if (!ActiveRequest.Snapshot)
	{
		CompleteActiveWrite(false);
		return;
	}
	FAsyncSaveGameToSlotDelegate saveDelegate;
	saveDelegate.BindUObject(this, &ULRSaveSubsystem::HandleAsyncSaveFinished);
	UGameplayStatics::AsyncSaveGameToSlot(ActiveRequest.Snapshot, ActiveRequest.SlotName, 0, saveDelegate);
}

/**
 * @brief 按 Save 调优延迟重新提交当前不可变快照；超过最大次数后报告失败并推进队列。
 */
void ULRSaveSubsystem::RetryActiveWrite()
{
	if (bWriteInProgress)
	{
		StartActiveWrite();
	}
}

/**
 * @brief 处理 Handle Async Save Finished 事件，将引擎回调转换为对应领域状态更新。
 * @param slotName 实际磁盘槽名称，由自动槽或手动槽规则生成。
 * @param userIndex 本次操作使用的计数、增量或索引 `userIndex`；由函数校验合法范围。
 * @param bSuccess 布尔开关 `bSuccess`；true 表示启用或条件成立，false 表示禁用或条件不成立。
 */
void ULRSaveSubsystem::HandleAsyncSaveFinished(const FString& slotName, const int32 userIndex, const bool bSuccess)
{
	CompleteActiveWrite(bSuccess);
}

/**
 * @brief 处理当前异步写入结果；成功后推进事务，失败时按调优重试或返回错误。
 * @param bSuccess 布尔开关 `bSuccess`；true 表示启用或条件成立，false 表示禁用或条件不成立。
 */
void ULRSaveSubsystem::CompleteActiveWrite(const bool bSuccess)
{
	if (!bWriteInProgress)
	{
		return;
	}
	if (!bSuccess && ActiveRequest.RetryAttempt < GetEffectiveTuning().RetryCount)
	{
		++ActiveRequest.RetryAttempt;
		UWorld* world = GetCurrentWorld();
		if (world)
		{
			world->GetTimerManager().SetTimer(RetryTimer, this, &ULRSaveSubsystem::RetryActiveWrite,
				GetEffectiveTuning().RetryDelaySeconds, false);
			return;
		}
	}

	const FName reasonId = ActiveRequest.ReasonId;
	const ELRSaveWriteKind writeKind = ActiveRequest.Kind;
	if (bSuccess)
	{
		UE_LOG(LogLostRunicSave, Log, TEXT("Save completed slot=%s reason=%s kind=%d retries=%d"),
			*ActiveRequest.SlotName, *reasonId.ToString(), static_cast<int32>(writeKind), ActiveRequest.RetryAttempt);
	}
	else
	{
		UE_LOG(LogLostRunicSave, Warning, TEXT("Save failed slot=%s reason=%s kind=%d retries=%d"),
			*ActiveRequest.SlotName, *reasonId.ToString(), static_cast<int32>(writeKind), ActiveRequest.RetryAttempt);
	}
	OnSaveWriteCompleted.Broadcast(reasonId, bSuccess);
	UpdateMemoryPhaseAfterWrite(writeKind, bSuccess);
	ActiveRequest = FLRQueuedSaveRequest();
	bWriteInProgress = false;
	StartNextWrite();
}

/**
 * @brief 执行 Load Slot 的持久化边界，并返回可诊断结果。
 * @param slotName 实际磁盘槽名称，由自动槽或手动槽规则生成。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
ELRSaveRequestResult ULRSaveSubsystem::LoadSlot(const FString& slotName)
{
	if (!UGameplayStatics::DoesSaveGameExist(slotName, 0))
	{
		OnSaveLoadCompleted.Broadcast(slotName, false, TEXT("Save does not exist."));
		return ELRSaveRequestResult::MissingOrCorrupt;
	}
	ULRSaveGame* loadedSave = Cast<ULRSaveGame>(UGameplayStatics::LoadGameFromSlot(slotName, 0));
	FString error;
	if (!loadedSave || !loadedSave->MigrateToLatest(error))
	{
		const FString failure = error.IsEmpty() ? TEXT("Save is corrupt or uses an unsupported class.") : error;
		UE_LOG(LogLostRunicSave, Warning, TEXT("Save load failed slot=%s error=%s"), *slotName, *failure);
		OnSaveLoadCompleted.Broadcast(slotName, false, failure);
		return ELRSaveRequestResult::MissingOrCorrupt;
	}
	WorkingSave = loadedSave;
	bAwaitingLoadedResume = false;
	const FName currentMapId = GetCurrentMapId();
	if (WorkingSave->ResumeAnchor.IsValid() && currentMapId != WorkingSave->ResumeAnchor.MapId)
	{
		if (!TravelToMap(WorkingSave->ResumeAnchor.MapId))
		{
			const FString failure = FString::Printf(TEXT("Resume map '%s' is unavailable."),
				*WorkingSave->ResumeAnchor.MapId.ToString());
			UE_LOG(LogLostRunicSave, Warning, TEXT("Save load rejected slot=%s error=%s"), *slotName, *failure);
			OnSaveLoadCompleted.Broadcast(slotName, false, failure);
			return ELRSaveRequestResult::RejectedUnavailableMap;
		}
		bAwaitingLoadedResume = true;
	}
	else
	{
		ApplyRuntimeState(Cast<ALRCharacter>(UGameplayStatics::GetPlayerCharacter(GetCurrentWorld(), 0)));
	}
	OnSaveLoadCompleted.Broadcast(slotName, true, FString());
	return ELRSaveRequestResult::Loaded;
}

/**
 * @brief 执行 Load Manual Slot 的持久化边界，并返回可诊断结果。
 * @param manualSlotIndex 本次操作使用的计数、增量或索引 `manualSlotIndex`；由函数校验合法范围。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
ELRSaveRequestResult ULRSaveSubsystem::LoadManualSlot(const int32 manualSlotIndex)
{
	return LRSaveRules::IsManualSlotValid(manualSlotIndex, GetManualSlotCount())
		? LoadSlot(LRSaveRules::MakeSlotName(ELRSaveSlotType::Manual, manualSlotIndex))
		: ELRSaveRequestResult::RejectedInvalidSlot;
}

/**
 * @brief 比较所有有效槽位时间戳并加载最新存档。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
ELRSaveRequestResult ULRSaveSubsystem::ContinueLatestSave()
{
	FString latestSlot;
	FDateTime latestTime;
	const auto considerSlot = [this, &latestSlot, &latestTime](const FString& slotName)
	{
		ULRSaveGame* save = Cast<ULRSaveGame>(UGameplayStatics::LoadGameFromSlot(slotName, 0));
		if (save && save->LastSavedUtc > latestTime)
		{
			latestTime = save->LastSavedUtc;
			latestSlot = slotName;
		}
	};
	const FString autoSlot = LRSaveRules::MakeSlotName(ELRSaveSlotType::Auto);
	if (UGameplayStatics::DoesSaveGameExist(autoSlot, 0))
	{
		considerSlot(autoSlot);
	}
	for (int32 slotIndex = 0; slotIndex < GetManualSlotCount(); ++slotIndex)
	{
		const FString slotName = LRSaveRules::MakeSlotName(ELRSaveSlotType::Manual, slotIndex);
		if (UGameplayStatics::DoesSaveGameExist(slotName, 0))
		{
			considerSlot(slotName);
		}
	}
	return latestSlot.IsEmpty() ? ELRSaveRequestResult::MissingOrCorrupt : LoadSlot(latestSlot);
}
