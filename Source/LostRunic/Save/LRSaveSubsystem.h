/** @file LRSaveSubsystem.h @brief The single V2 persistence dispatcher. */
#pragma once

#include "Save/LRSaveTypes.h"
#include "Save/LRSaveV2Types.h"
#include "Save/LRSaveProvider.h"
#include "Narrative/LRNarrativeTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "LRSaveSubsystem.generated.h"

class ULRSaveCatalog;
class ULRSavePayload;
class ULRSaveTuning;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRSaveOperationCompleted, FLRSaveOperationResult, result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FLRSaveOperationStarted, FGuid, gameFlowTransactionId, FGuid, operationId,
	ELRSaveOperationType, operation, FLRSaveSlotId, slotId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FLRSaveOperationPhaseChanged, FGuid, gameFlowTransactionId, FGuid, operationId,
	ELRSaveOperationState, state, ELRSaveOperationType, operation, FLRSaveSlotId, slotId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRSaveLoadRequested, FLRSaveFlowRequest, request);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRSaveNewGameRequested, FLRSaveFlowRequest, request);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRSaveCatalogStateChanged, ELRSaveCatalogState, state);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRSaveCatalogSnapshotChanged, FLRSaveCatalogSnapshot, snapshot);

UCLASS(meta = (DisplayName = "Lost Runic Save Subsystem"))
class LOSTRUNIC_API ULRSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Save")
	TArray<FLRSaveSlotMetadata> GetSaveSlots() const;

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Save|Catalog")
	ELRSaveCatalogState GetCatalogState() const { return CatalogState; }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Save|Catalog")
	bool IsCatalogReady() const { return CatalogState == ELRSaveCatalogState::Ready; }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Save|Catalog")
	bool HasAnyCatalogEntry() const;

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Save|Catalog")
	bool CanContinue() const;

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Save|Catalog")
	FLRSaveCatalogSnapshot GetCatalogSnapshot() const { return CatalogSnapshot; }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Save")
	int32 GetMaxManualSaveSlots() const;

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Save")
	FLRSaveOperationResult RequestCreateManualSave(FName reasonId);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Save")
	FLRSaveOperationResult RequestOverwriteSave(FLRSaveSlotId slotId, FName reasonId);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Save")
	FLRSaveOperationResult RequestAutoSave(FName reasonId);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Save")
	FLRSaveOperationResult RequestLoadSave(FLRSaveSlotId slotId);
	FLRSaveOperationResult RequestLoadSaveForFlow(FLRSaveSlotId slotId, FGuid gameFlowTransactionId);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Save")
	FLRSaveOperationResult RequestDeleteSave(FLRSaveSlotId slotId);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Save")
	FLRSaveOperationResult RequestContinue();
	FLRSaveOperationResult RequestContinueForFlow(FGuid gameFlowTransactionId);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Save")
	FLRSaveOperationResult RequestNewGame();
	FLRSaveOperationResult RequestNewGameForFlow(FGuid gameFlowTransactionId);

	bool CaptureProviderState(FLRSaveDataV2& outData, FString& outError);
	bool RestoreProviderState(const FLRSaveDataV2& data, FString& outError);
	bool ResetProvidersForNewGame(FString& outError);
	FLRSaveOperationResult RequestCriticalSaveFromSnapshot(const FLRSaveDataV2& snapshot,
		const FLRNarrativePersistentState& committedState,
		const FLRNarrativePersistentDelta& narrativeDelta, FName reasonId, FGuid gameFlowTransactionId,
		ELRSaveMemoryPurpose memoryPurpose, FGuid requestedOperationId = FGuid());

	void NotifyLoadWorldReady(FGuid gameFlowTransactionId, FGuid operationId);
	void NotifyLoadPreparationFailed(FGuid gameFlowTransactionId, FGuid operationId, const FString& diagnostic);
	void NotifyNewGameWorldReady(FGuid gameFlowTransactionId, FGuid operationId);

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Save")
	ELRSaveOperationState GetOperationState() const { return OperationState; }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Save")
	bool IsManualSaveAllowed() const;

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Save")
	void SetResumeAnchor(const FLRResumeAnchor& anchor);

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Save")
	FLRResumeAnchor GetResumeAnchor() const;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Save")
	FLRSaveOperationCompleted OnSaveOperationCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Save")
	FLRSaveOperationStarted OnSaveOperationStarted;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Save")
	FLRSaveOperationPhaseChanged OnSaveOperationPhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Save")
	FLRSaveLoadRequested OnSaveLoadRequested;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Save")
	FLRSaveNewGameRequested OnSaveNewGameRequested;


	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Save|Catalog")
	FLRSaveCatalogStateChanged OnCatalogStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Save|Catalog")
	FLRSaveCatalogSnapshotChanged OnCatalogSnapshotChanged;

private:
	FLRSaveOperationResult EnqueueOperation(ELRSaveOperationType type, const FLRSaveSlotId& slotId,
		FName reasonId, const FLRSaveDataV2* capturedData = nullptr,
		ELRSaveMemoryPurpose memoryPurpose = ELRSaveMemoryPurpose::None,
		ELRSaveSlotHealth requestedHealth = ELRSaveSlotHealth::Healthy, bool bFront = false,
		FGuid requestedOperationId = FGuid(), FGuid gameFlowTransactionId = FGuid(),
		bool bDeferStart = false);
	FLRSaveOperationResult MakeRejected(ELRSaveOperationType type, const FLRSaveSlotId& slotId,
		ELRSaveResultCode code, const FString& diagnostic, FGuid requestedOperationId = FGuid(),
		FGuid gameFlowTransactionId = FGuid()) const;
	bool CaptureCurrentData(FLRSaveDataV2& outData, FString& outError);
	void CapturePendingAutoSave();
	void StartNextOperation();
	void ScheduleStartNextOperation();
	void SetOperationState(ELRSaveOperationState newState);
	void DispatchActiveOperation();
	void StartWrite();
	void StartLoad();
	void StartNewGame();
	void StartDelete();
	void StartRepairHealth();
	void HandlePayloadWritten(FGuid operationId, const FString& slotName, int32 userIndex, bool bSuccess);
	void HandleOperationTimeout(FGuid operationId);
	void HandleAsyncWatchdog(FGuid operationId);
	void RetryActiveOperation(FGuid operationId);
	void CompleteOperation(ELRSaveResultCode code, const FString& diagnostic = FString());
	void CancelQueuedOperations(const FString& diagnostic);
	void EnqueueHealthRepair(const FLRSaveSlotId& slotId, ELRSaveSlotHealth health);
	void EnqueuePendingCatalogRepair();
	void SetCatalogState(ELRSaveCatalogState newState);
	void PublishCatalogSnapshot();

	void HandleNarrativeEventCommitted(const FLRStoryEventCommit& eventCommit);
	FName GetCurrentMapId() const;
	UWorld* GetCurrentWorld() const;
	const ULRSaveTuning& GetEffectiveTuning() const;
	int32 GetManualSlotCount() const;
	FLRSaveSlotMetadata BuildMetadata(const FLRSaveSlotId& slotId, int32 displayIndex,
		int64 saveSequence, const FLRSaveDataV2& data, const FString& payloadKey) const;

	UPROPERTY(Transient)
	TObjectPtr<ULRSaveTuning> Tuning;
	UPROPERTY(Transient)
	TObjectPtr<ULRSaveCatalog> SaveCatalog;
	UPROPERTY(Transient)
	TObjectPtr<ULRSavePayload> ActivePayload;

	TArray<TUniquePtr<ILRSaveProvider>> SaveProviders;
	TArray<FLRQueuedSaveOperation> OperationQueue;
	FLRQueuedSaveOperation ActiveOperation;
	ELRSaveOperationState OperationState = ELRSaveOperationState::Idle;
	ELRSaveCatalogState CatalogState = ELRSaveCatalogState::Initializing;
	FLRSaveCatalogSnapshot CatalogSnapshot;
	FLRSaveDataV2 CurrentData;
	bool bPersistenceBlocked = false;
	FName PendingAutoSaveReason = NAME_None;
	FGuid PendingAutoSaveOperationId;

	FTimerHandle AutoSaveDebounceTimer;
	FTimerHandle ExplicitRetryTimer;
	FTimerHandle OperationTimeoutTimer;
	FTimerHandle AsyncWatchdogTimer;
	bool bOperationStartDeferred = false;
};
