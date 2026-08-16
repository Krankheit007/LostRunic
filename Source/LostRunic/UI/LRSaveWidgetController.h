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

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Save UI|Focus")
	FLRSaveFocusTarget GetFocusTarget() const { return Snapshot.FocusTarget; }

	void UpdateFocusTarget(const FLRSaveFocusTarget& focusTarget);

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Save UI")
	FLRSaveUISnapshotChanged OnSnapshotChanged;

	static FLRSaveUISnapshot BuildSnapshot(const TArray<FLRSaveSlotMetadata>& slots,
		ELRSaveSelectionMode mode, ELRSaveUIState state, bool bManualSaveAllowed,
		int32 maxManualSlots, const ULRGameContentSet* contentSet);
	static FLRSaveFocusTarget ReconcileFocusTarget(const FLRSaveUISnapshot& snapshot,
		const FLRSaveFocusTarget& requestedTarget);
	/** Resolves Primary/Delete semantics without performing storage I/O. */
	static ELRSaveOperationType ResolveSlotAction(ELRSaveSelectionMode mode,
		const FLRSaveSlotMetadata& slot, bool bDeleteRequest, bool bManualSaveAllowed = true);

private:
	void Refresh();
	void SubmitOperation(ELRSaveOperationType operation, const FLRSaveSlotId& slotId = FLRSaveSlotId());
	void AcceptSubmission(const FLRSaveOperationResult& result);
	void ApplyCompletion(const FLRSaveOperationResult& result);
	void SetError(ELRSaveResultCode code);
	bool FindSlot(const FLRSaveSlotId& slotId, FLRSaveSlotMetadata& outSlot) const;
	FText ResolveText(FName textKey) const;
	void UpdateConfirmationViewModel();

	UFUNCTION()
	void HandleOperationCompleted(FLRSaveOperationResult result);

	UFUNCTION()
	void HandleCatalogStateChanged(ELRSaveCatalogState state);

	UFUNCTION()
	void HandleCatalogSnapshotChanged(FLRSaveCatalogSnapshot snapshot);

	UPROPERTY(Transient) TObjectPtr<ULRSaveSubsystem> SaveSubsystem;
	UPROPERTY(Transient) TObjectPtr<const ULRGameContentSet> ContentSet;
	FLRSaveUISnapshot Snapshot;
	FGuid PendingOperationId;
	FLRSaveSlotId PendingSlotId;
	ELRSaveOperationType ExpectedOperation = ELRSaveOperationType::None;
	TOptional<FLRSaveOperationResult> SubmissionCompletion;
	bool bOpen = false;
	bool bSubmittingRequest = false;
};
