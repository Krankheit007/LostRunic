#pragma once

#include "Save/LRSaveTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "LRSaveSubsystem.generated.h"

class ALRCharacter;
class ULRSaveGame;
class ULRSaveTuning;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLRSaveWriteQueued, FName, reasonId, ELRSaveWriteKind, writeKind);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLRSaveWriteCompleted, FName, reasonId, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FLRSaveLoadCompleted, FString, slotName, bool, bSuccess, FString, error);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRMemoryTransactionChanged, ELRMemoryTransactionPhase, phase);

/** Owns versioned save snapshots, FIFO async writes, and death-to-memory persistence transactions. */
UCLASS(meta = (DisplayName = "Lost Runic Save Subsystem"))
class LOSTRUNIC_API ULRSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Save")
	void SetResumeAnchor(const FLRResumeAnchor& anchor);

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Save")
	FLRResumeAnchor GetResumeAnchor() const;

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Save")
	ELRSaveRequestResult RequestAutoSave(FName reasonId);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Save")
	ELRSaveRequestResult RequestManualSave(int32 manualSlotIndex, FName reasonId);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Save")
	ELRSaveRequestResult RequestCriticalSave(FName reasonId);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Save")
	ELRSaveRequestResult LoadManualSlot(int32 manualSlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Save")
	ELRSaveRequestResult ContinueLatestSave();

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Save")
	bool IsManualSaveAllowed() const;

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Save")
	bool IsWriteInProgress() const { return bWriteInProgress; }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Save")
	ELRMemoryTransactionPhase GetMemoryPhase() const { return MemoryPhase; }

	bool BeginDeathMemoryTransaction(ALRCharacter* character);
	bool CommitMemoryEvent(FName eventId);
	bool RequestReturnFromMemory();
	void HandleWorldReady(ALRCharacter* character);

	const ULRSaveGame* GetWorkingSave() const { return WorkingSave; }

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Save")
	FLRSaveWriteQueued OnSaveWriteQueued;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Save")
	FLRSaveWriteCompleted OnSaveWriteCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Save")
	FLRSaveLoadCompleted OnSaveLoadCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Save|Memory")
	FLRMemoryTransactionChanged OnMemoryTransactionChanged;

private:
	UFUNCTION()
	void HandleNarrativeEventCommitted(FName eventId, ELRSavePolicy savePolicy);

	void CaptureRuntimeState();
	void ApplyRuntimeState(ALRCharacter* character);
	ULRSaveGame* CreateSnapshot() const;
	ELRSaveRequestResult LoadSlot(const FString& slotName);
	ELRSaveRequestResult QueueWrite(const FString& slotName, FName reasonId, ELRSaveWriteKind writeKind);
	void QueuePendingAutoSave();
	void StartNextWrite();
	void StartActiveWrite();
	void RetryActiveWrite();
	void HandleAsyncSaveFinished(const FString& slotName, int32 userIndex, bool bSuccess);
	void CompleteActiveWrite(bool bSuccess);
	void UpdateMemoryPhaseAfterWrite(ELRSaveWriteKind writeKind, bool bSuccess);
	const ULRSaveTuning& GetEffectiveTuning() const;
	int32 GetManualSlotCount() const;
	UWorld* GetCurrentWorld() const;
	FName GetCurrentMapId() const;
	bool TravelToMap(FName mapId);
	void SetMemoryPhase(ELRMemoryTransactionPhase newPhase);
	void SetTransitionInput(bool bVisible) const;
	void ApplyMemoryState(ALRCharacter* character) const;

	UPROPERTY(Transient)
	TObjectPtr<ULRSaveGame> WorkingSave;

	UPROPERTY(Transient)
	TObjectPtr<ULRSaveTuning> Tuning;

	UPROPERTY(Transient)
	TArray<FLRQueuedSaveRequest> RequestQueue;

	UPROPERTY(Transient)
	FLRQueuedSaveRequest ActiveRequest;

	FName PendingAutoSaveReason = NAME_None;
	ELRMemoryTransactionPhase MemoryPhase = ELRMemoryTransactionPhase::None;
	bool bWriteInProgress = false;
	bool bAwaitingLoadedResume = false;
	FTimerHandle AutoSaveDebounceTimer;
	FTimerHandle RetryTimer;
};
