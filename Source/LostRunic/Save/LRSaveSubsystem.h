/** @file LRSaveSubsystem.h @brief The single V2 persistence dispatcher. */
#pragma once

#include "Save/LRSaveTypes.h"
#include "Save/LRSaveV2Types.h"
#include "Save/LRSaveProvider.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "LRSaveSubsystem.generated.h"

class ALRCharacter;
class ULRSaveCatalog;
class ULRSavePayload;
class ULRSaveTuning;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRSaveOperationCompleted, FLRSaveOperationResult, result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLRSaveLoadRequested, FGuid, operationId, FName, mapId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLRSaveNewGameRequested, FGuid, operationId, FName, mapId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRMemoryTransactionChanged, ELRMemoryTransactionPhase, phase);
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

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Save")
	FLRSaveOperationResult RequestDeleteSave(FLRSaveSlotId slotId);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Save")
	FLRSaveOperationResult RequestContinue();

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Save")
	FLRSaveOperationResult RequestNewGame();

	void NotifyLoadWorldReady(FGuid operationId);
	void NotifyLoadPreparationFailed(FGuid operationId, const FString& diagnostic);
	void NotifyNewGameWorldReady(FGuid operationId);

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Save")
	ELRSaveOperationState GetOperationState() const { return OperationState; }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Save")
	bool IsManualSaveAllowed() const;

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Save")
	void SetResumeAnchor(const FLRResumeAnchor& anchor);

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Save")
	FLRResumeAnchor GetResumeAnchor() const;

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Save|Memory")
	ELRMemoryTransactionPhase GetMemoryPhase() const { return MemoryPhase; }

	bool BeginDeathMemoryTransaction(ALRCharacter* character);
	bool CommitMemoryEvent(FName eventId);
	bool RequestReturnFromMemory();
	void HandleWorldReady(ALRCharacter* character);

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Save")
	FLRSaveOperationCompleted OnSaveOperationCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Save")
	FLRSaveLoadRequested OnSaveLoadRequested;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Save")
	FLRSaveNewGameRequested OnSaveNewGameRequested;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Save|Memory")
	FLRMemoryTransactionChanged OnMemoryTransactionChanged;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Save|Catalog")
	FLRSaveCatalogStateChanged OnCatalogStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Save|Catalog")
	FLRSaveCatalogSnapshotChanged OnCatalogSnapshotChanged;

private:
	FLRSaveOperationResult EnqueueOperation(ELRSaveOperationType type, const FLRSaveSlotId& slotId,
		FName reasonId, const FLRSaveDataV2* capturedData = nullptr,
		ELRSaveMemoryPurpose memoryPurpose = ELRSaveMemoryPurpose::None,
		ELRSaveSlotHealth requestedHealth = ELRSaveSlotHealth::Healthy, bool bFront = false,
		FGuid requestedOperationId = FGuid());
	FLRSaveOperationResult MakeRejected(ELRSaveOperationType type, const FLRSaveSlotId& slotId,
		ELRSaveResultCode code, const FString& diagnostic) const;
	bool CaptureCurrentData(FLRSaveDataV2& outData, FString& outError);
	void CapturePendingAutoSave();
	void StartNextOperation();
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

	UFUNCTION()
	void HandleNarrativeEventCommitted(FName eventId, ELRSavePolicy savePolicy);

	void UpdateMemoryPhaseAfterOperation(const FLRQueuedSaveOperation& operation, bool bSuccess);
	void SetMemoryPhase(ELRMemoryTransactionPhase newPhase);
	void SetTransitionInput(bool bVisible) const;
	void ApplyMemoryState(ALRCharacter* character) const;
	void ApplyDataToRuntime(const FLRSaveDataV2& data, ALRCharacter* character);
	FName GetCurrentMapId() const;
	UWorld* GetCurrentWorld() const;
	bool TravelToMap(FName mapId);
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
	FLRSaveDataV2 HomeResumeSnapshot;
	ELRMemoryTransactionPhase MemoryPhase = ELRMemoryTransactionPhase::None;
	bool bHasHomeResumeSnapshot = false;
	bool bPersistenceBlocked = false;
	FName PendingAutoSaveReason = NAME_None;
	FGuid PendingAutoSaveOperationId;
	FGuid PendingNewGameOperationId;

	FTimerHandle AutoSaveDebounceTimer;
	FTimerHandle ExplicitRetryTimer;
	FTimerHandle OperationTimeoutTimer;
	FTimerHandle AsyncWatchdogTimer;
};
