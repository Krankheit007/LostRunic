/** @file LRSaveConfirmDialogWidget.h @brief 覆盖/删除确认对话框 Widget 父类。 */
#pragma once

#include "Blueprint/UserWidget.h"
#include "UI/LRSaveUITypes.h"

#include "LRSaveConfirmDialogWidget.generated.h"

class UButton;
class UPanelWidget;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLRSaveConfirmActionRequested);

UCLASS(Abstract, Blueprintable, meta = (DisplayName = "Lost Runic Save Confirm Dialog Widget"))
class LOSTRUNIC_API ULRSaveConfirmDialogWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Save UI")
	void ApplyViewModel(const FLRSaveConfirmViewModel& viewModel);
	bool FocusDefaultAction();

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Save UI")
	FLRSaveConfirmActionRequested OnConfirmRequested;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Save UI")
	FLRSaveConfirmActionRequested OnCancelRequested;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Lost Runic|Save UI")
	void OnViewModelChanged(const FLRSaveConfirmViewModel& viewModel);

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UPanelWidget> Cover;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UPanelWidget> Delete;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UButton> Cover_Confirm;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UButton> Cover_Cancel;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UButton> Delete_Confirm;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UButton> Delete_Cancel;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UTextBlock> MessageText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UTextBlock> Cover_Confirm_T;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UTextBlock> Cover_Cancel_T;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UTextBlock> Delete_T_1;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UTextBlock> Delete_T;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UTextBlock> Delete_Confirm_T;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UTextBlock> Delete_Cancel_T;
	UPROPERTY(BlueprintReadOnly, Category = "Save UI") FLRSaveConfirmViewModel ViewModel;

	UFUNCTION()
	void HandleConfirmClicked();

	UFUNCTION()
	void HandleCancelClicked();
};
