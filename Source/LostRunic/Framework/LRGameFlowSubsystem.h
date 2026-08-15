/** @file LRGameFlowSubsystem.h @brief Event-driven map travel and save restoration coordinator. */
#pragma once

#include "Save/LRSaveV2Types.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "LRGameFlowSubsystem.generated.h"

class ALRCharacter;

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
	bool IsTravelOrRestoreInProgress() const { return PendingLoadOperationId.IsValid(); }

private:
	UFUNCTION()
	void HandleLoadRequested(FGuid operationId, FName mapId);

	UFUNCTION()
	void HandleNewGameRequested(FGuid operationId, FName mapId);

	UFUNCTION()
	void HandleSaveOperationCompleted(FLRSaveOperationResult result);

	bool TravelToMap(FName mapId);

	FGuid PendingLoadOperationId;
	FName PendingLoadMapId = NAME_None;
	FGuid PendingNewGameOperationId;
	FName PendingNewGameMapId = NAME_None;
};
