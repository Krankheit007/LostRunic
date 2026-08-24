/** @file LRGameFlowSubsystem.h @brief Event-driven map travel and save restoration coordinator. */
#pragma once

#include "Narrative/LRNarrativeTypes.h"
#include "Save/LRSaveV2Types.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "LRGameFlowSubsystem.generated.h"

class ALRCharacter;

UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Game Flow Phase"))
enum class ELRGameFlowPhase : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	PreparingTravel UMETA(DisplayName = "Preparing Travel"),
	Traveling UMETA(DisplayName = "Traveling"),
	WaitingForWorld UMETA(DisplayName = "Waiting For World"),
	Restoring UMETA(DisplayName = "Restoring"),
	EnteringMemory UMETA(DisplayName = "Entering Memory"),
	ReturningFromMemory UMETA(DisplayName = "Returning From Memory")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FLRGameFlowPhaseChanged, FGuid, gameFlowTransactionId,
	FGuid, saveOperationId, ELRGameFlowPhase, phase, FName, mapId);

UCLASS(meta = (DisplayName = "Lost Runic Game Flow Subsystem"))
class LOSTRUNIC_API ULRGameFlowSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& collection) override;
	virtual void Deinitialize() override;

	/** Called by GameMode after the destination world and its player have initialized. */
	void NotifyWorldReady(ALRCharacter* character);

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Game Flow")
	bool IsTravelOrRestoreInProgress() const { return FlowPhase != ELRGameFlowPhase::Idle; }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Game Flow")
	ELRGameFlowPhase GetFlowPhase() const { return FlowPhase; }

	/** The UI asks GameFlow to create the transaction; Save only queues the correlated operation. */
	FLRSaveOperationResult RequestLoadSave(const FLRSaveSlotId& slotId);
	FLRSaveOperationResult RequestContinue();
	FLRSaveOperationResult RequestNewGame();

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Game Flow")
	bool TravelToMainMenu();

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Game Flow")
	bool RequestEnterMemory(ALRCharacter* character);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Game Flow")
	bool RequestCommitMemoryEvent(FName eventId);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Game Flow")
	bool RequestReturnFromMemory();

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Game Flow")
	FLRGameFlowPhaseChanged OnFlowPhaseChanged;

private:
	UFUNCTION()
	void HandleLoadRequested(FLRSaveFlowRequest request);

	UFUNCTION()
	void HandleNewGameRequested(FLRSaveFlowRequest request);

	UFUNCTION()
	void HandleSaveOperationCompleted(FLRSaveOperationResult result);

	bool TravelToMap(FName mapId);
	bool ValidateFlowRequest(const FLRSaveFlowRequest& request, FString& outError) const;
	bool QueueMemoryCriticalSave(ELRSaveMemoryPurpose purpose, FName reasonId);
	bool RestoreHomeStateForReturn();
	void SetFlowPhase(FGuid gameFlowTransactionId, ELRGameFlowPhase newPhase, FName mapId);
	void SetFlowPhase(FGuid gameFlowTransactionId, FGuid saveOperationId, ELRGameFlowPhase newPhase, FName mapId);
	void ClearStandardFlow();
	void ClearMemoryTransaction();
	FName GetCurrentMapId() const;

	FLRSaveFlowRequest ActiveFlowRequest;
	ELRGameFlowPhase FlowPhase = ELRGameFlowPhase::Idle;
	FName ActiveMapId = NAME_None;

	FLRSaveDataV2 HomeSnapshot;
	FLRNarrativePersistentDelta DurableNarrativeDelta;
	bool bHasHomeSnapshot = false;
	bool bMemoryWorldActive = false;
	FGuid MemoryTransactionId;
	FGuid PendingMemorySaveOperationId;
	ELRSaveMemoryPurpose PendingMemoryPurpose = ELRSaveMemoryPurpose::None;
};
