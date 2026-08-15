#include "UI/LRMainMenuWidgetController.h"

#include "Save/LRSaveSubsystem.h"

void ULRMainMenuWidgetController::Initialize(ULRSaveSubsystem* saveSubsystem)
{
	if (!saveSubsystem || SaveSubsystem == saveSubsystem)
	{
		return;
	}
	Deinitialize();
	SaveSubsystem = saveSubsystem;
	SaveSubsystem->OnSaveOperationCompleted.AddDynamic(this, &ULRMainMenuWidgetController::HandleSaveOperationCompleted);
	SaveSubsystem->OnCatalogStateChanged.AddDynamic(this, &ULRMainMenuWidgetController::HandleCatalogStateChanged);
	Refresh();
}

void ULRMainMenuWidgetController::Deinitialize()
{
	if (SaveSubsystem)
	{
		SaveSubsystem->OnSaveOperationCompleted.RemoveDynamic(this, &ULRMainMenuWidgetController::HandleSaveOperationCompleted);
		SaveSubsystem->OnCatalogStateChanged.RemoveDynamic(this, &ULRMainMenuWidgetController::HandleCatalogStateChanged);
	}
	SaveSubsystem = nullptr;
	PendingOperationId.Invalidate();
}

void ULRMainMenuWidgetController::RequestNewGame()
{
	if (!SaveSubsystem || !ViewModel.bCanNewGame || PendingOperationId.IsValid()) return;
	const FLRSaveOperationResult result = SaveSubsystem->RequestNewGame();
	if (result.Code == ELRSaveResultCode::Queued) PendingOperationId = result.OperationId;
	Refresh();
}

void ULRMainMenuWidgetController::RequestContinue()
{
	if (!SaveSubsystem || !ViewModel.bCanContinue || PendingOperationId.IsValid()) return;
	const FLRSaveOperationResult result = SaveSubsystem->RequestContinue();
	if (result.Code == ELRSaveResultCode::Queued) PendingOperationId = result.OperationId;
	Refresh();
}

void ULRMainMenuWidgetController::RequestExit()
{
	// Exit is intentionally a presentation-owned action; the Host decides platform policy.
}

void ULRMainMenuWidgetController::Refresh()
{
	if (!SaveSubsystem) return;
	ViewModel = FLRMainMenuViewModel();
	ViewModel.CatalogState = SaveSubsystem->GetCatalogState();
	ViewModel.bIsBusy = PendingOperationId.IsValid() || SaveSubsystem->GetOperationState() != ELRSaveOperationState::Idle;
	ViewModel.bCanNewGame = SaveSubsystem->IsCatalogReady() && !ViewModel.bIsBusy;
	ViewModel.bCanContinue = SaveSubsystem->CanContinue() && !ViewModel.bIsBusy;
	ViewModel.bCanLoad = SaveSubsystem->HasAnyCatalogEntry() && !ViewModel.bIsBusy;
	ViewModel.bCanOptions = false;
	ViewModel.bCanExit = true;
	OnViewChanged.Broadcast(ViewModel);
}

void ULRMainMenuWidgetController::HandleSaveOperationCompleted(const FLRSaveOperationResult result)
{
	if (PendingOperationId.IsValid() && result.OperationId == PendingOperationId) PendingOperationId.Invalidate();
	Refresh();
}

void ULRMainMenuWidgetController::HandleCatalogStateChanged(const ELRSaveCatalogState state)
{
	Refresh();
}
