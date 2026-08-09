/**
 * @file LRSaveSubsystemMemory.cpp
 * @brief 实现一个自动槽、十个手动槽、版本迁移、不可变快照、FIFO 异步写入，以及死亡进入 Memory 和返回恢复锚点的 A/B 关键事务。
 *
 * 关联文件：Save 目录内调用该公共契约的实现文件；所属领域：Save。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Save/LRSaveSubsystem.h"

#include "Core/LRGameplayTags.h"
#include "Core/LRLog.h"
#include "Data/LRGameContentSet.h"
#include "Framework/LRCharacter.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "Framework/LRPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Save/LRSaveGame.h"
#include "Save/LRSaveRules.h"
#include "State/LRStateComponent.h"
#include "UI/LRHUD.h"

/**
 * @brief 开始 Begin Death Memory Transaction 流程，建立本次操作拥有的状态、委托或计时器。
 * @param character 参与本次操作的运行时对象 `character`；函数会检查空值和所需接口。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRSaveSubsystem::BeginDeathMemoryTransaction(ALRCharacter* character)
{
	if (!WorkingSave || !LRSaveRules::CanBeginMemoryTransaction(MemoryPhase, WorkingSave->ResumeAnchor))
	{
		UE_LOG(LogLostRunicSave, Warning, TEXT("Memory transaction rejected phase=%d anchor=%s"),
			static_cast<int32>(MemoryPhase), WorkingSave && WorkingSave->ResumeAnchor.IsValid() ? TEXT("valid") : TEXT("invalid"));
		return false;
	}
	CaptureRuntimeState();
	++WorkingSave->Narrative.DeathCount;
	SetMemoryPhase(ELRMemoryTransactionPhase::AwaitingMemoryWorld);
	SetTransitionInput(true);
	if (TravelToMap(LRSaveIds::MemoryMapId))
	{
		return true;
	}
	SetMemoryPhase(ELRMemoryTransactionPhase::None);
	SetTransitionInput(false);
	return false;
}

/**
 * @brief 在 Memory 事务中记录一次调查事件并排队关键进度写入，不覆盖恢复锚点。
 * @param eventId 剧情事件的稳定 FName ID，用于一次性判定和存档。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRSaveSubsystem::CommitMemoryEvent(const FName eventId)
{
	if (MemoryPhase != ELRMemoryTransactionPhase::InMemory || eventId.IsNone() || !WorkingSave)
	{
		return false;
	}
	if (WorkingSave->Narrative.MemoryEventIds.Contains(eventId))
	{
		return false;
	}
	WorkingSave->Narrative.MemoryEventIds.Add(eventId);
	return QueueWrite(LRSaveRules::MakeSlotName(ELRSaveSlotType::Auto), eventId, ELRSaveWriteKind::MemoryEvent)
		== ELRSaveRequestResult::Queued;
}

/**
 * @brief 结束 Memory 调查并切回恢复锚点地图，等待世界应用完毕后提交关键存档 B。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRSaveSubsystem::RequestReturnFromMemory()
{
	if (!WorkingSave || MemoryPhase != ELRMemoryTransactionPhase::InMemory || !WorkingSave->ResumeAnchor.IsValid())
	{
		return false;
	}
	SetMemoryPhase(ELRMemoryTransactionPhase::AwaitingResumeWorld);
	SetTransitionInput(true);
	if (TravelToMap(WorkingSave->ResumeAnchor.MapId))
	{
		return true;
	}
	SetMemoryPhase(ELRMemoryTransactionPhase::InMemory);
	SetTransitionInput(false);
	return false;
}

/**
 * @brief 处理 Handle World Ready 事件，将引擎回调转换为对应领域状态更新。
 * @param character 参与本次操作的运行时对象 `character`；函数会检查空值和所需接口。
 */
void ULRSaveSubsystem::HandleWorldReady(ALRCharacter* character)
{
	const FName currentMapId = GetCurrentMapId();
	if (bAwaitingLoadedResume && WorkingSave && currentMapId == WorkingSave->ResumeAnchor.MapId)
	{
		bAwaitingLoadedResume = false;
		ApplyRuntimeState(character);
		return;
	}
	if (LRSaveRules::IsMemoryEntryWorld(MemoryPhase, currentMapId))
	{
		ApplyMemoryState(character);
		SetMemoryPhase(ELRMemoryTransactionPhase::SavingEntry);
		QueueWrite(LRSaveRules::MakeSlotName(ELRSaveSlotType::Auto), LRSaveIds::MemoryEntryReason,
			ELRSaveWriteKind::MemoryEntry);
		return;
	}
	if (WorkingSave && LRSaveRules::IsResumeWorld(MemoryPhase, currentMapId, WorkingSave->ResumeAnchor))
	{
		ApplyRuntimeState(character);
		SetMemoryPhase(ELRMemoryTransactionPhase::SavingReturn);
		QueueWrite(LRSaveRules::MakeSlotName(ELRSaveSlotType::Auto), LRSaveIds::MemoryReturnReason,
			ELRSaveWriteKind::MemoryReturn);
	}
}

/**
 * @brief 根据最新领域状态刷新 Update Memory Phase After Write，并仅在值变化时通知订阅者。
 * @param writeKind 本次操作使用的 `writeKind` 枚举或模式值。
 * @param bSuccess 布尔开关 `bSuccess`；true 表示启用或条件成立，false 表示禁用或条件不成立。
 */
void ULRSaveSubsystem::UpdateMemoryPhaseAfterWrite(const ELRSaveWriteKind writeKind, const bool bSuccess)
{
	ELRMemoryTransactionPhase nextPhase = LRSaveRules::ResolveAfterWrite(MemoryPhase, writeKind, bSuccess);
	if (!bSuccess && writeKind == ELRSaveWriteKind::MemoryEntry)
	{
		nextPhase = ELRMemoryTransactionPhase::InMemory;
	}
	if (!bSuccess && writeKind == ELRSaveWriteKind::MemoryReturn)
	{
		nextPhase = ELRMemoryTransactionPhase::None;
	}
	if (nextPhase == MemoryPhase)
	{
		return;
	}
	SetMemoryPhase(nextPhase);
	if (nextPhase == ELRMemoryTransactionPhase::InMemory || nextPhase == ELRMemoryTransactionPhase::None)
	{
		SetTransitionInput(false);
	}
}

/**
 * @brief 查询 Current Map Id；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FName ULRSaveSubsystem::GetCurrentMapId() const
{
	const ULRGameInstanceSubsystem* dataSubsystem = GetGameInstance()->GetSubsystem<ULRGameInstanceSubsystem>();
	const ULRGameContentSet* contentSet = dataSubsystem ? dataSubsystem->GetContentSet() : nullptr;
	return contentSet ? contentSet->FindMapIdForWorld(GetCurrentWorld()) : NAME_None;
}

/**
 * @brief 按 LRGameContentSet 中注册的地图 ID 异步/同步发起关卡切换，不拼接硬编码资产路径。
 * @param mapId 稳定标识 `mapId`；用于内容查询和存档，不依赖显示名或数组序号。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRSaveSubsystem::TravelToMap(const FName mapId)
{
	const ULRGameInstanceSubsystem* dataSubsystem = GetGameInstance()->GetSubsystem<ULRGameInstanceSubsystem>();
	const ULRGameContentSet* contentSet = dataSubsystem ? dataSubsystem->GetContentSet() : nullptr;
	const TSoftObjectPtr<UWorld> map = contentSet ? contentSet->FindMap(mapId) : TSoftObjectPtr<UWorld>();
	if (map.IsNull())
	{
		UE_LOG(LogLostRunicSave, Warning, TEXT("Save travel rejected map=%s is not registered."), *mapId.ToString());
		return false;
	}
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, map);
	return true;
}

/**
 * @brief 更新 Memory Phase，并在需要时同步组件状态或广播变化事件。
 * @param newPhase 本次操作使用的 `newPhase` 枚举或模式值。
 */
void ULRSaveSubsystem::SetMemoryPhase(const ELRMemoryTransactionPhase newPhase)
{
	if (MemoryPhase == newPhase)
	{
		return;
	}
	MemoryPhase = newPhase;
	OnMemoryTransactionChanged.Broadcast(MemoryPhase);
	UE_LOG(LogLostRunicSave, Log, TEXT("Memory transaction phase=%d"), static_cast<int32>(MemoryPhase));
}

/**
 * @brief 更新 Transition Input，并在需要时同步组件状态或广播变化事件。
 * @param bVisible 布尔开关 `bVisible`；true 表示启用或条件成立，false 表示禁用或条件不成立。
 */
void ULRSaveSubsystem::SetTransitionInput(const bool bVisible) const
{
	ALRPlayerController* controller = Cast<ALRPlayerController>(UGameplayStatics::GetPlayerController(GetCurrentWorld(), 0));
	if (!controller)
	{
		return;
	}
	if (ALRHUD* hud = controller->GetHUD<ALRHUD>())
	{
		hud->ShowTransition(bVisible);
	}
	controller->SetLRInputMode(bVisible ? ELRInputMode::Transition : ELRInputMode::Gameplay);
}

/**
 * @brief 把 Apply Memory State 数据应用到运行时对象，并显式处理缺失依赖。
 * @param character 参与本次操作的运行时对象 `character`；函数会检查空值和所需接口。
 */
void ULRSaveSubsystem::ApplyMemoryState(ALRCharacter* character) const
{
	ULRStateComponent* state = character ? character->GetStateComponent() : nullptr;
	if (!state || state->GetCurrentMode() == ELRPerceptionMode::Memory)
	{
		return;
	}
	FLRStateChangeRequest request;
	request.TargetMode = ELRPerceptionMode::Memory;
	request.RequestType = ELRStateRequestType::Death;
	request.Source = LRGameplayTags::StateSourceDeath;
	state->RequestStateChange(request);
}
