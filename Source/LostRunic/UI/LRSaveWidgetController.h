/** @file LRSaveWidgetController.h @brief Event-driven state machine and view model for V2 save screens. */
#pragma once

#include "UI/LRSaveUITypes.h"
#include "UObject/Object.h"

#include "LRSaveWidgetController.generated.h"

class ULRGameContentSet;
class ULRSaveSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRSaveUISnapshotChanged, const FLRSaveUISnapshot&, snapshot);

UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Save Widget Controller"))
class LOSTRUNIC_API ULRSaveWidgetController : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(ULRSaveSubsystem* saveSubsystem, const ULRGameContentSet* contentSet);
	void Deinitialize();

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Save UI")
	void Open(ELRSaveSelectionMode mode);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Save UI")
	void Close();

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Save UI")
	void RequestCreateManualSave();

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Save UI")
	void RequestPrimarySlotAction(FLRSaveSlotId slotId);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Save UI")
	void RequestDelete(FLRSaveSlotId slotId);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Save UI")
	void ConfirmPendingAction();

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Save UI")
	void CancelPendingAction();

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Save UI")
	void DismissError();

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Save UI")
	const FLRSaveUISnapshot& GetSnapshot() const { return Snapshot; }

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Save UI")
	FLRSaveUISnapshotChanged OnSnapshotChanged;

	static FLRSaveUISnapshot BuildSnapshot(const TArray<FLRSaveSlotMetadata>& slots,
		ELRSaveSelectionMode mode, ELRSaveUIState state, bool bManualSaveAllowed,
		int32 maxManualSlots, const ULRGameContentSet* contentSet);

private:
	void Refresh();
	void SubmitOperation(ELRSaveOperationType operation, const FLRSaveSlotId& slotId = FLRSaveSlotId());
	void AcceptSubmission(const FLRSaveOperationResult& result);
	void ApplyCompletion(const FLRSaveOperationResult& result);
	void SetError(ELRSaveResultCode code);
	bool FindSlot(const FLRSaveSlotId& slotId, FLRSaveSlotMetadata& outSlot) const;

	UFUNCTION()
	void HandleOperationCompleted(FLRSaveOperationResult result);

	UPROPERTY(Transient) TObjectPtr<ULRSaveSubsystem> SaveSubsystem;
	UPROPERTY(Transient) TObjectPtr<const ULRGameContentSet> ContentSet;
	FLRSaveUISnapshot Snapshot;
	FGuid PendingOperationId;
	ELRSaveOperationType ExpectedOperation = ELRSaveOperationType::None;
	TOptional<FLRSaveOperationResult> SubmissionCompletion;
	bool bOpen = false;
	bool bSubmittingRequest = false;
};
