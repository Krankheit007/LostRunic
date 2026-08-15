/** @file LRMainMenuWidgetController.h @brief 主菜单按钮的轻量事件控制器。 */
#pragma once

#include "UI/LRMainMenuUITypes.h"
#include "UObject/Object.h"

#include "LRMainMenuWidgetController.generated.h"

class ULRSaveSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRMainMenuViewChanged, const FLRMainMenuViewModel&, viewModel);

UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Main Menu Widget Controller"))
class LOSTRUNIC_API ULRMainMenuWidgetController : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(ULRSaveSubsystem* saveSubsystem);
	void Deinitialize();

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Main Menu") void RequestNewGame();
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Main Menu") void RequestContinue();
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Main Menu") void RequestExit();
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Main Menu") const FLRMainMenuViewModel& GetViewModel() const { return ViewModel; }

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Main Menu") FLRMainMenuViewChanged OnViewChanged;

private:
	void Refresh();
	UFUNCTION() void HandleSaveOperationCompleted(FLRSaveOperationResult result);
	UFUNCTION() void HandleCatalogStateChanged(ELRSaveCatalogState state);

	UPROPERTY(Transient) TObjectPtr<ULRSaveSubsystem> SaveSubsystem;
	FLRMainMenuViewModel ViewModel;
	FGuid PendingOperationId;
};
