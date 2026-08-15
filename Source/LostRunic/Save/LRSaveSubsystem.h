/**
 * @file LRSaveSubsystem.h
 * @brief 管理槽位元数据、快照构建、普通自动存档防抖、失败重试、异步 FIFO 队列、继续游戏选择及 Memory A/B 事务。
 *
 * 关联文件：LRSaveSubsystem.cpp；所属领域：Save。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Save/LRSaveTypes.h"
#include "Save/LRSaveV2Types.h"
#include "Save/LRSaveProvider.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "LRSaveSubsystem.generated.h"

class ALRCharacter;
class ULRSaveGame;
class ULRSaveCatalog;
class ULRSavePayload;
class ULRSaveTuning;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLRSaveWriteQueued, FName, reasonId, ELRSaveWriteKind, writeKind);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLRSaveWriteCompleted, FName, reasonId, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FLRSaveLoadCompleted, FString, slotName, bool, bSuccess, FString, error);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRMemoryTransactionChanged, ELRMemoryTransactionPhase, phase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRSaveOperationCompleted, FLRSaveOperationResult, result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLRSaveLoadRequested, FGuid, operationId, FName, mapId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLRSaveNewGameRequested, FGuid, operationId, FName, mapId);

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(meta = (DisplayName = "Lost Runic Save Subsystem"))
class LOSTRUNIC_API ULRSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * @brief 初始化子系统拥有的长期状态与事件绑定。
	 * @param collection 调用方提供的 `collection`，只在本次操作范围内使用。
	 */
	virtual void Initialize(FSubsystemCollectionBase& collection) override;
	/**
	 * @brief 释放子系统事件绑定和运行时缓存。
	 */
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Save|V2")
	TArray<FLRSaveSlotMetadata> GetSaveSlots() const;

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Save|V2")
	int32 GetMaxManualSaveSlots() const { return GetManualSlotCount(); }

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Save|V2")
	FLRSaveOperationResult RequestCreateManualSave(FName reasonId);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Save|V2")
	FLRSaveOperationResult RequestOverwriteSave(FLRSaveSlotId slotId, FName reasonId);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Save|V2")
	FLRSaveOperationResult RequestAutoSaveV2(FName reasonId);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Save|V2")
	FLRSaveOperationResult RequestLoadSave(FLRSaveSlotId slotId);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Save|V2")
	FLRSaveOperationResult RequestDeleteSave(FLRSaveSlotId slotId);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Save|V2")
	FLRSaveOperationResult RequestContinue();

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Save|V2")
	FLRSaveOperationResult RequestNewGame();

	/** Called by GameFlow after travel and provider initialization, never from Save itself. */
	void NotifyLoadWorldReady(FGuid operationId);
	void NotifyLoadPreparationFailed(FGuid operationId, const FString& diagnostic);
	void NotifyNewGameWorldReady(FGuid operationId);

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Save|V2")
	ELRSaveOperationState GetOperationState() const { return V2OperationState; }

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Save|V2")
	FLRSaveOperationCompleted OnSaveOperationCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Save|V2")
	FLRSaveLoadRequested OnSaveLoadRequested;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Save|V2")
	FLRSaveNewGameRequested OnSaveNewGameRequested;

	/**
	 * @brief 更新 Resume Anchor，并在需要时同步组件状态或广播变化事件。
	 * @param anchor 调用方提供的 `anchor`，只在本次操作范围内使用。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Save")
	void SetResumeAnchor(const FLRResumeAnchor& anchor);

	/**
	 * @brief 查询 Resume Anchor；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Save")
	FLRResumeAnchor GetResumeAnchor() const;

	/**
	 * @brief 请求普通自动存档；按 Save 调优资产执行防抖，不合并关键 Memory 事务。
	 * @param reasonId 稳定标识 `reasonId`；用于内容查询和存档，不依赖显示名或数组序号。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Save")
	ELRSaveRequestResult RequestAutoSave(FName reasonId);

	/**
	 * @brief 请求写入指定手动槽；校验槽位范围，并在 Memory 事务期间明确拒绝。
	 * @param manualSlotIndex 本次操作使用的计数、增量或索引 `manualSlotIndex`；由函数校验合法范围。
	 * @param reasonId 稳定标识 `reasonId`；用于内容查询和存档，不依赖显示名或数组序号。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Save")
	ELRSaveRequestResult RequestManualSave(int32 manualSlotIndex, FName reasonId);

	/**
	 * @brief 创建不可变快照并将关键写入直接加入 FIFO，不参与普通自动存档防抖。
	 * @param reasonId 稳定标识 `reasonId`；用于内容查询和存档，不依赖显示名或数组序号。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Save")
	ELRSaveRequestResult RequestCriticalSave(FName reasonId);

	/**
	 * @brief 执行 Load Manual Slot 的持久化边界，并返回可诊断结果。
	 * @param manualSlotIndex 本次操作使用的计数、增量或索引 `manualSlotIndex`；由函数校验合法范围。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Save")
	ELRSaveRequestResult LoadManualSlot(int32 manualSlotIndex);

	/**
	 * @brief 比较所有有效槽位时间戳并加载最新存档。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Save")
	ELRSaveRequestResult ContinueLatestSave();

	/**
	 * @brief 判断 Is Manual Save Allowed 对应条件；不产生玩法副作用。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Save")
	bool IsManualSaveAllowed() const;

	/**
	 * @brief 判断 Is Write In Progress 对应条件；不产生玩法副作用。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Save")
	bool IsWriteInProgress() const { return bWriteInProgress; }

	/**
	 * @brief 查询 Memory Phase；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Save")
	ELRMemoryTransactionPhase GetMemoryPhase() const { return MemoryPhase; }

	/**
	 * @brief 开始 Begin Death Memory Transaction 流程，建立本次操作拥有的状态、委托或计时器。
	 * @param character 参与本次操作的运行时对象 `character`；函数会检查空值和所需接口。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool BeginDeathMemoryTransaction(ALRCharacter* character);
	/**
	 * @brief 在 Memory 事务中记录一次调查事件并排队关键进度写入，不覆盖恢复锚点。
	 * @param eventId 剧情事件的稳定 FName ID，用于一次性判定和存档。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool CommitMemoryEvent(FName eventId);
	/**
	 * @brief 结束 Memory 调查并切回恢复锚点地图，等待世界应用完毕后提交关键存档 B。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool RequestReturnFromMemory();
	/**
	 * @brief 处理 Handle World Ready 事件，将引擎回调转换为对应领域状态更新。
	 * @param character 参与本次操作的运行时对象 `character`；函数会检查空值和所需接口。
	 */
	void HandleWorldReady(ALRCharacter* character);

	/**
	 * @brief 查询 Working Save；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	const ULRSaveGame* GetWorkingSave() const { return WorkingSave; }

	/** 当 Save Write Queued 发生时广播；蓝图可绑定该委托以更新表现，不应在回调中改写核心规则。  */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Save")
	FLRSaveWriteQueued OnSaveWriteQueued;

	/** 当 Save Write Completed 发生时广播；蓝图可绑定该委托以更新表现，不应在回调中改写核心规则。  */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Save")
	FLRSaveWriteCompleted OnSaveWriteCompleted;

	/** 当 Save Load Completed 发生时广播；蓝图可绑定该委托以更新表现，不应在回调中改写核心规则。  */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Save")
	FLRSaveLoadCompleted OnSaveLoadCompleted;

	/** 当 Memory Transaction Changed 发生时广播；蓝图可绑定该委托以更新表现，不应在回调中改写核心规则。  */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Save|Memory")
	FLRMemoryTransactionChanged OnMemoryTransactionChanged;

private:
	FLRSaveOperationResult EnqueueV2Operation(ELRSaveOperationType type, const FLRSaveSlotId& slotId,
		FName reasonId = NAME_None);
	void StartNextV2Operation();
	void StartV2Write();
	void HandleV2PayloadWritten(const FString& slotName, int32 userIndex, bool bSuccess);
	void StartV2Load();
	void StartV2NewGame();
	void StartV2Delete();
	void CompleteV2Operation(ELRSaveResultCode code, const FString& diagnostic = FString());
	bool PersistDeterministicHealth(const FLRSaveSlotId& slotId, ELRSaveSlotHealth health);
	FLRSaveSlotMetadata BuildV2Metadata(const FLRSaveSlotId& slotId, int32 displayIndex,
		int64 saveSequence, const ULRSavePayload& payload) const;
	bool PrepareV2Payload(FLRQueuedSaveOperation& operation, int32 displayIndex, FString& outError);
	/**
	 * @brief 处理 Handle Narrative Event Committed 事件，将引擎回调转换为对应领域状态更新。
	 * @param eventId 剧情事件的稳定 FName ID，用于一次性判定和存档。
	 * @param savePolicy 本次操作使用的 `savePolicy` 枚举或模式值。
	 */
	UFUNCTION()
	void HandleNarrativeEventCommitted(FName eventId, ELRSavePolicy savePolicy);

	/**
	 * @brief 从角色组件捕获位置、状态、库存和剧情进度，更新内存中的存档模型。
	 */
	void CaptureRuntimeState();
	/**
	 * @brief 把 Apply Runtime State 数据应用到运行时对象，并显式处理缺失依赖。
	 * @param character 参与本次操作的运行时对象 `character`；函数会检查空值和所需接口。
	 */
	void ApplyRuntimeState(ALRCharacter* character);
	/**
	 * @brief 根据当前领域状态构建 Create Snapshot 所需的数据，不把临时对象作为长期存档标识。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	ULRSaveGame* CreateSnapshot() const;
	/**
	 * @brief 执行 Load Slot 的持久化边界，并返回可诊断结果。
	 * @param slotName 实际磁盘槽名称，由自动槽或手动槽规则生成。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	ELRSaveRequestResult LoadSlot(const FString& slotName);
	/**
	 * @brief 按既定顺序将 Queue Write 请求加入队列，保留调用时快照。
	 * @param slotName 实际磁盘槽名称，由自动槽或手动槽规则生成。
	 * @param reasonId 稳定标识 `reasonId`；用于内容查询和存档，不依赖显示名或数组序号。
	 * @param writeKind 本次操作使用的 `writeKind` 枚举或模式值。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	ELRSaveRequestResult QueueWrite(const FString& slotName, FName reasonId, ELRSaveWriteKind writeKind);
	/**
	 * @brief 按既定顺序将 Queue Pending Auto Save 请求加入队列，保留调用时快照。
	 */
	void QueuePendingAutoSave();
	/**
	 * @brief 开始 Start Next Write 流程，建立本次操作拥有的状态、委托或计时器。
	 */
	void StartNextWrite();
	/**
	 * @brief 启动 FIFO 队首快照的异步写盘，并保留重试次数和请求种类。
	 */
	void StartActiveWrite();
	/**
	 * @brief 按 Save 调优延迟重新提交当前不可变快照；超过最大次数后报告失败并推进队列。
	 */
	void RetryActiveWrite();
	/**
	 * @brief 处理 Handle Async Save Finished 事件，将引擎回调转换为对应领域状态更新。
	 * @param slotName 实际磁盘槽名称，由自动槽或手动槽规则生成。
	 * @param userIndex 本次操作使用的计数、增量或索引 `userIndex`；由函数校验合法范围。
	 * @param bSuccess 布尔开关 `bSuccess`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 */
	void HandleAsyncSaveFinished(const FString& slotName, int32 userIndex, bool bSuccess);
	/**
	 * @brief 处理当前异步写入结果；成功后推进事务，失败时按调优重试或返回错误。
	 * @param bSuccess 布尔开关 `bSuccess`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 */
	void CompleteActiveWrite(bool bSuccess);
	/**
	 * @brief 根据最新领域状态刷新 Update Memory Phase After Write，并仅在值变化时通知订阅者。
	 * @param writeKind 本次操作使用的 `writeKind` 枚举或模式值。
	 * @param bSuccess 布尔开关 `bSuccess`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 */
	void UpdateMemoryPhaseAfterWrite(ELRSaveWriteKind writeKind, bool bSuccess);
	/**
	 * @brief 查询 Effective Tuning；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	const ULRSaveTuning& GetEffectiveTuning() const;
	/**
	 * @brief 查询 Manual Slot Count；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	int32 GetManualSlotCount() const;
	/**
	 * @brief 查询 Current World；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UWorld* GetCurrentWorld() const;
	/**
	 * @brief 查询 Current Map Id；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	FName GetCurrentMapId() const;
	/**
	 * @brief 按 LRGameContentSet 中注册的地图 ID 异步/同步发起关卡切换，不拼接硬编码资产路径。
	 * @param mapId 稳定标识 `mapId`；用于内容查询和存档，不依赖显示名或数组序号。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool TravelToMap(FName mapId);
	/**
	 * @brief 更新 Memory Phase，并在需要时同步组件状态或广播变化事件。
	 * @param newPhase 本次操作使用的 `newPhase` 枚举或模式值。
	 */
	void SetMemoryPhase(ELRMemoryTransactionPhase newPhase);
	/**
	 * @brief 更新 Transition Input，并在需要时同步组件状态或广播变化事件。
	 * @param bVisible 布尔开关 `bVisible`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 */
	void SetTransitionInput(bool bVisible) const;
	/**
	 * @brief 把 Apply Memory State 数据应用到运行时对象，并显式处理缺失依赖。
	 * @param character 参与本次操作的运行时对象 `character`；函数会检查空值和所需接口。
	 */
	void ApplyMemoryState(ALRCharacter* character) const;

	/** Working Save 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TObjectPtr<ULRSaveGame> WorkingSave;

	/** 运行时解析出的调优资产缓存；不序列化，不由蓝图编辑。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TObjectPtr<ULRSaveTuning> Tuning;

	UPROPERTY(Transient)
	TObjectPtr<ULRSaveCatalog> SaveCatalog;

	UPROPERTY(Transient)
	TObjectPtr<ULRSavePayload> LoadedV2Payload;

	TArray<TUniquePtr<ILRSaveProvider>> SaveProviders;
	TArray<FLRQueuedSaveOperation> V2OperationQueue;
	FLRQueuedSaveOperation ActiveV2Operation;
	ELRSaveOperationState V2OperationState = ELRSaveOperationState::Idle;

	/** Request Queue 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TArray<FLRQueuedSaveRequest> RequestQueue;

	/** Active Request 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	FLRQueuedSaveRequest ActiveRequest;

	/** Pending Auto Save Reason 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	FName PendingAutoSaveReason = NAME_None;
	/** Memory Phase 的内部运行时数据；不参与蓝图配置。 */
	ELRMemoryTransactionPhase MemoryPhase = ELRMemoryTransactionPhase::None;
	/** Write In Progress 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	bool bWriteInProgress = false;
	/** Awaiting Loaded Resume 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	bool bAwaitingLoadedResume = false;
	FGuid PendingNewGameOperationId;
	/** Auto Save Debounce Timer 的运行时句柄，用于取消回调并避免 Tick；不在蓝图中配置。 */
	FTimerHandle AutoSaveDebounceTimer;
	/** Retry Timer 的运行时句柄，用于取消回调并避免 Tick；不在蓝图中配置。 */
	FTimerHandle RetryTimer;
};
