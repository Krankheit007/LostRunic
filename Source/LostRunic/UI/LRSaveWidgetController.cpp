#include "UI/LRSaveWidgetController.h"

#include "Data/LRGameContentSet.h"
#include "Save/LRSaveSubsystem.h"

namespace
{
	bool IsBusyState(const ELRSaveUIState state)
	{
		return state == ELRSaveUIState::Saving || state == ELRSaveUIState::Loading
			|| state == ELRSaveUIState::Deleting;
	}

	ELRSaveUIState StateForOperation(const ELRSaveOperationType operation)
	{
		if (operation == ELRSaveOperationType::Load || operation == ELRSaveOperationType::Continue)
		{
			return ELRSaveUIState::Loading;
		}
		return operation == ELRSaveOperationType::Delete ? ELRSaveUIState::Deleting : ELRSaveUIState::Saving;
	}
}

void ULRSaveWidgetController::Initialize(ULRSaveSubsystem* saveSubsystem, const ULRGameContentSet* contentSet)
{
	if (!saveSubsystem || SaveSubsystem == saveSubsystem)
	{
		return;
	}
	Deinitialize();
	SaveSubsystem = saveSubsystem;
	ContentSet = contentSet;
	SaveSubsystem->OnSaveOperationCompleted.AddDynamic(this, &ULRSaveWidgetController::HandleOperationCompleted);
	SaveSubsystem->OnCatalogStateChanged.AddDynamic(this, &ULRSaveWidgetController::HandleCatalogStateChanged);
	SaveSubsystem->OnCatalogSnapshotChanged.AddDynamic(this, &ULRSaveWidgetController::HandleCatalogSnapshotChanged);
}

void ULRSaveWidgetController::Deinitialize()
{
	if (SaveSubsystem)
	{
		SaveSubsystem->OnSaveOperationCompleted.RemoveDynamic(this, &ULRSaveWidgetController::HandleOperationCompleted);
		SaveSubsystem->OnCatalogStateChanged.RemoveDynamic(this, &ULRSaveWidgetController::HandleCatalogStateChanged);
		SaveSubsystem->OnCatalogSnapshotChanged.RemoveDynamic(this, &ULRSaveWidgetController::HandleCatalogSnapshotChanged);
	}
	SaveSubsystem = nullptr;
	ContentSet = nullptr;
	bOpen = false;
	PendingOperationId.Invalidate();
	PendingSlotId = FLRSaveSlotId();
	ExpectedOperation = ELRSaveOperationType::None;
}

void ULRSaveWidgetController::Open(const ELRSaveSelectionMode mode)
{
	bOpen = true;
	Snapshot.Mode = mode;
	Snapshot.State = PendingOperationId.IsValid() || bSubmittingRequest
		? StateForOperation(ExpectedOperation)
		: (SaveSubsystem && SaveSubsystem->GetCatalogState() == ELRSaveCatalogState::Blocked
			? ELRSaveUIState::Error
			: (SaveSubsystem && !SaveSubsystem->IsCatalogReady() ? ELRSaveUIState::LoadingCatalog : ELRSaveUIState::Idle));
	Snapshot.Confirmation = ELRSaveUIConfirmation::None;
	Snapshot.StatusMessage = FText::GetEmpty();
	Snapshot.FocusTarget = FLRSaveFocusTarget::MakeRoot();
	UpdateConfirmationViewModel();
	Refresh();
}

void ULRSaveWidgetController::Close()
{
	bOpen = false;
	Snapshot.Confirmation = ELRSaveUIConfirmation::None;
	Snapshot.SelectedSlotId = FLRSaveSlotId();
	Snapshot.FocusTarget = FLRSaveFocusTarget::MakeRoot();
	UpdateConfirmationViewModel();
}

void ULRSaveWidgetController::RequestCreateManualSave()
{
	if (Snapshot.Mode == ELRSaveSelectionMode::Save && Snapshot.State == ELRSaveUIState::Idle
		&& Snapshot.bCanCreateManualSlot)
	{
		Snapshot.FocusTarget = FLRSaveFocusTarget::MakeCreate(Snapshot.CreateDisplayIndex);
		SubmitOperation(ELRSaveOperationType::CreateManual);
	}
}

void ULRSaveWidgetController::RequestPrimarySlotAction(const FLRSaveSlotId slotId)
{
	FLRSaveSlotMetadata slot;
	if (!FindSlot(slotId, slot) || Snapshot.State != ELRSaveUIState::Idle)
	{
		return;
	}
	Snapshot.FocusTarget = FLRSaveFocusTarget::MakeExisting(slotId);
	if (Snapshot.Mode == ELRSaveSelectionMode::Load && slot.Health == ELRSaveSlotHealth::Healthy)
	{
		SubmitOperation(ELRSaveOperationType::Load, slotId);
	}
	else if (Snapshot.Mode == ELRSaveSelectionMode::Save && slotId.Type == ELRSaveSlotType::Manual)
	{
		Snapshot.State = ELRSaveUIState::Confirming;
		Snapshot.Confirmation = ELRSaveUIConfirmation::Overwrite;
		Snapshot.SelectedSlotId = slotId;
		UpdateConfirmationViewModel();
		OnSnapshotChanged.Broadcast(Snapshot);
	}
}

void ULRSaveWidgetController::RequestDelete(const FLRSaveSlotId slotId)
{
	FLRSaveSlotMetadata slot;
	if (!FindSlot(slotId, slot) || slotId.Type != ELRSaveSlotType::Manual
		|| Snapshot.State != ELRSaveUIState::Idle)
	{
		return;
	}
	Snapshot.State = ELRSaveUIState::Confirming;
	Snapshot.Confirmation = ELRSaveUIConfirmation::Delete;
	Snapshot.SelectedSlotId = slotId;
	Snapshot.FocusTarget = FLRSaveFocusTarget::MakeExisting(slotId);
	UpdateConfirmationViewModel();
	OnSnapshotChanged.Broadcast(Snapshot);
}

void ULRSaveWidgetController::ConfirmPendingAction()
{
	if (Snapshot.State != ELRSaveUIState::Confirming || !Snapshot.SelectedSlotId.IsValid())
	{
		return;
	}
	const ELRSaveOperationType operation = Snapshot.Confirmation == ELRSaveUIConfirmation::Overwrite
		? ELRSaveOperationType::OverwriteManual : ELRSaveOperationType::Delete;
	SubmitOperation(operation, Snapshot.SelectedSlotId);
}

void ULRSaveWidgetController::CancelPendingAction()
{
	if (Snapshot.State != ELRSaveUIState::Confirming)
	{
		return;
	}
	Snapshot.State = ELRSaveUIState::Idle;
	Snapshot.Confirmation = ELRSaveUIConfirmation::None;
	Snapshot.SelectedSlotId = FLRSaveSlotId();
	UpdateConfirmationViewModel();
	OnSnapshotChanged.Broadcast(Snapshot);
}

void ULRSaveWidgetController::DismissError()
{
	if (Snapshot.State == ELRSaveUIState::Error)
	{
		Snapshot.State = ELRSaveUIState::Idle;
		Snapshot.StatusMessage = FText::GetEmpty();
		Refresh();
	}
}

FLRSaveUISnapshot ULRSaveWidgetController::BuildSnapshot(const TArray<FLRSaveSlotMetadata>& slots,
	const ELRSaveSelectionMode mode, const ELRSaveUIState state, const bool bManualSaveAllowed,
	const int32 maxManualSlots, const ULRGameContentSet* contentSet)
{
	FLRSaveUISnapshot result;
	result.Mode = mode;
	result.State = state;
	result.bIsBusy = IsBusyState(state);
	int32 manualCount = 0;
	for (const FLRSaveSlotMetadata& metadata : slots)
	{
		FLRSaveSlotView& view = result.Slots.AddDefaulted_GetRef();
		view.SlotId = metadata.SlotId;
		view.DisplayIndex = metadata.DisplayIndex;
		view.SavedAtUtc = metadata.SavedAtUtc;
		view.PlayTimeSeconds = metadata.PlayTimeSeconds;
		view.CollectedCount = metadata.CollectedCount;
		view.TotalCollectibleCount = contentSet ? contentSet->GetTotalCollectibleCount() : 0;
		view.Health = metadata.Health;
		view.bAutomatic = metadata.SlotId.Type == ELRSaveSlotType::Auto;
		view.bCanLoad = metadata.Health == ELRSaveSlotHealth::Healthy;
		view.bCanOverwrite = !view.bAutomatic && bManualSaveAllowed;
		view.bCanDelete = !view.bAutomatic;
		view.MapDisplayName = contentSet ? contentSet->GetMapDisplayName(metadata.MapId) : FText::FromName(metadata.MapId);
		manualCount += view.bAutomatic ? 0 : 1;
	}
	result.Slots.Sort([](const FLRSaveSlotView& a, const FLRSaveSlotView& b)
	{
		if (a.bAutomatic != b.bAutomatic)
		{
			return a.bAutomatic;
		}
		return a.DisplayIndex < b.DisplayIndex;
	});
	result.bCanCreateManualSlot = mode == ELRSaveSelectionMode::Save && state == ELRSaveUIState::Idle
		&& bManualSaveAllowed && manualCount < maxManualSlots;
	result.CreateDisplayIndex = manualCount + 1;
	return result;
}

void ULRSaveWidgetController::Refresh()
{
	if (!SaveSubsystem || !bOpen)
	{
		return;
	}
	if (SaveSubsystem->GetCatalogState() == ELRSaveCatalogState::Blocked)
	{
		Snapshot.Slots.Reset();
		Snapshot.State = ELRSaveUIState::Error;
		Snapshot.bIsBusy = false;
		Snapshot.StatusMessage = NSLOCTEXT("LRSaveUI", "CatalogBlocked", "Save data is unavailable.");
		UpdateConfirmationViewModel();
		OnSnapshotChanged.Broadcast(Snapshot);
		return;
	}
	if (!SaveSubsystem->IsCatalogReady())
	{
		Snapshot.Slots.Reset();
		Snapshot.State = ELRSaveUIState::LoadingCatalog;
		Snapshot.bIsBusy = true;
		Snapshot.StatusMessage = FText::GetEmpty();
		UpdateConfirmationViewModel();
		OnSnapshotChanged.Broadcast(Snapshot);
		return;
	}
	const ELRSaveSelectionMode mode = Snapshot.Mode;
	const ELRSaveUIState state = Snapshot.State;
	const FText status = Snapshot.StatusMessage;
	const FLRSaveFocusTarget focusTarget = Snapshot.FocusTarget;
	const FLRSaveSlotId selectedSlotId = Snapshot.SelectedSlotId;
	const ELRSaveUIConfirmation confirmation = Snapshot.Confirmation;
	Snapshot = BuildSnapshot(SaveSubsystem->GetSaveSlots(), mode, state, SaveSubsystem->IsManualSaveAllowed(),
		SaveSubsystem->GetMaxManualSaveSlots(), ContentSet);
	Snapshot.StatusMessage = status;
	Snapshot.FocusTarget = focusTarget.IsValid() ? focusTarget : FLRSaveFocusTarget::MakeRoot();
	Snapshot.SelectedSlotId = selectedSlotId;
	Snapshot.Confirmation = confirmation;
	UpdateConfirmationViewModel();
	OnSnapshotChanged.Broadcast(Snapshot);
}

void ULRSaveWidgetController::SubmitOperation(const ELRSaveOperationType operation, const FLRSaveSlotId& slotId)
{
	if (!SaveSubsystem || IsBusyState(Snapshot.State))
	{
		return;
	}
	Snapshot.State = StateForOperation(operation);
	Snapshot.Confirmation = ELRSaveUIConfirmation::None;
	UpdateConfirmationViewModel();
	Snapshot.bIsBusy = true;
	ExpectedOperation = operation;
	bSubmittingRequest = true;
	SubmissionCompletion.Reset();
	OnSnapshotChanged.Broadcast(Snapshot);

	FLRSaveOperationResult result;
	if (operation == ELRSaveOperationType::CreateManual)
		result = SaveSubsystem->RequestCreateManualSave(TEXT("ManualUI"));
	else if (operation == ELRSaveOperationType::OverwriteManual)
		result = SaveSubsystem->RequestOverwriteSave(slotId, TEXT("ManualUI"));
	else if (operation == ELRSaveOperationType::Load)
		result = SaveSubsystem->RequestLoadSave(slotId);
	else
		result = SaveSubsystem->RequestDeleteSave(slotId);

	bSubmittingRequest = false;
	AcceptSubmission(result);
}

void ULRSaveWidgetController::AcceptSubmission(const FLRSaveOperationResult& result)
{
	if (!IsBusyState(Snapshot.State))
	{
		return;
	}
	if (result.Code != ELRSaveResultCode::Queued)
	{
		SetError(result.Code);
		return;
	}
	PendingOperationId = result.OperationId;
	PendingSlotId = result.SlotId;
	if (SubmissionCompletion.IsSet())
	{
		const FLRSaveOperationResult completed = SubmissionCompletion.GetValue();
		SubmissionCompletion.Reset();
		if (completed.OperationId == PendingOperationId)
		{
			ApplyCompletion(completed);
		}
	}
}

void ULRSaveWidgetController::SetError(const ELRSaveResultCode code)
{
	Snapshot.State = ELRSaveUIState::Error;
	Snapshot.bIsBusy = false;
	Snapshot.Confirmation = ELRSaveUIConfirmation::None;
	Snapshot.StatusMessage = code == ELRSaveResultCode::RejectedAtCapacity
		? NSLOCTEXT("LRSaveUI", "Capacity", "No manual save slots are available.")
		: NSLOCTEXT("LRSaveUI", "OperationFailed", "The save operation could not be completed.");
	UpdateConfirmationViewModel();
	PendingOperationId.Invalidate();
	PendingSlotId = FLRSaveSlotId();
	ExpectedOperation = ELRSaveOperationType::None;
	OnSnapshotChanged.Broadcast(Snapshot);
}

bool ULRSaveWidgetController::FindSlot(const FLRSaveSlotId& slotId, FLRSaveSlotMetadata& outSlot) const
{
	if (!SaveSubsystem)
	{
		return false;
	}
	const TArray<FLRSaveSlotMetadata> slots = SaveSubsystem->GetSaveSlots();
	const FLRSaveSlotMetadata* found = slots.FindByPredicate(
		[slotId](const FLRSaveSlotMetadata& slot) { return slot.SlotId == slotId; });
	if (!found)
	{
		return false;
	}
	outSlot = *found;
	return true;
}

void ULRSaveWidgetController::HandleOperationCompleted(const FLRSaveOperationResult result)
{
	if (bSubmittingRequest)
	{
		SubmissionCompletion = result;
		return;
	}
	if (!PendingOperationId.IsValid() || result.OperationId != PendingOperationId)
	{
		return;
	}
	if (PendingSlotId.IsValid() && result.SlotId.IsValid() && result.SlotId != PendingSlotId)
	{
		return;
	}
	ApplyCompletion(result);
}

void ULRSaveWidgetController::ApplyCompletion(const FLRSaveOperationResult& result)
{
	PendingOperationId.Invalidate();
	PendingSlotId = FLRSaveSlotId();
	ExpectedOperation = ELRSaveOperationType::None;
	if (result.Code != ELRSaveResultCode::Succeeded)
	{
		SetError(result.Code);
		return;
	}
	Snapshot.State = ELRSaveUIState::Idle;
	Snapshot.bIsBusy = false;
	Snapshot.SelectedSlotId = FLRSaveSlotId();
	Snapshot.StatusMessage = FText::GetEmpty();
	Refresh();
}

void ULRSaveWidgetController::UpdateConfirmationViewModel()
{
	Snapshot.ConfirmationViewModel.Confirmation = Snapshot.Confirmation;
	Snapshot.ConfirmationViewModel.SlotId = Snapshot.SelectedSlotId;
	Snapshot.ConfirmationViewModel.bVisible = Snapshot.Confirmation != ELRSaveUIConfirmation::None;
	Snapshot.ConfirmationViewModel.Message = Snapshot.Confirmation == ELRSaveUIConfirmation::Overwrite
		? NSLOCTEXT("LRSaveUI", "ConfirmOverwrite", "Overwrite this save slot?")
		: (Snapshot.Confirmation == ELRSaveUIConfirmation::Delete
			? NSLOCTEXT("LRSaveUI", "ConfirmDelete", "Delete this save slot?") : FText::GetEmpty());
}

void ULRSaveWidgetController::HandleCatalogStateChanged(const ELRSaveCatalogState state)
{
	if (!bOpen || PendingOperationId.IsValid() || bSubmittingRequest)
	{
		return;
	}
	if (state == ELRSaveCatalogState::Blocked)
	{
		Snapshot.State = ELRSaveUIState::Error;
	}
	else if (state != ELRSaveCatalogState::Ready)
	{
		Snapshot.State = ELRSaveUIState::LoadingCatalog;
	}
	Refresh();
}

void ULRSaveWidgetController::HandleCatalogSnapshotChanged(const FLRSaveCatalogSnapshot snapshot)
{
	if (bOpen && snapshot.State == ELRSaveCatalogState::Ready && !PendingOperationId.IsValid())
	{
		Refresh();
	}
}
