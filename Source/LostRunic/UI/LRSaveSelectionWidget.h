/** @file LRSaveSelectionWidget.h @brief 存档选择页面父类：绑定控制器快照，不拥有控制器生命周期。 */
#pragma once

#include "UI/LRScreenWidget.h"
#include "UI/LRSaveUITypes.h"

#include "LRSaveSelectionWidget.generated.h"

class UButton;
class ULRCreateSaveSlotWidget;
class ULRSaveConfirmDialogWidget;
class ULRSaveSlotWidget;
class UPanelWidget;
class USizeBox;
class UTextBlock;
class UWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLRSaveSelectionBackRequested);

UCLASS(Abstract, Blueprintable, meta = (DisplayName = "Lost Runic Save Selection Widget"))
class LOSTRUNIC_API ULRSaveSelectionWidget : public ULRScreenWidget
{
	GENERATED_BODY()

public:
	virtual void SetSaveWidgetController(ULRSaveWidgetController* controller) override;
	virtual bool HandleUICommand_Implementation(ELRUICommand command) override;
	virtual bool SetInitialFocus() override;
	virtual bool RestoreFocus() override;
	virtual void SetScreenVisible(bool bVisible) override;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Save UI")
	FLRSaveSelectionBackRequested OnBackRequested;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Lost Runic|Save UI")
	void OnSaveSnapshotChanged(const FLRSaveUISnapshot& snapshot);

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UPanelWidget> SlotListPanel;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UButton> BackButton;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(EditDefaultsOnly, Category = "Save UI|Widget Classes")
	TSubclassOf<ULRSaveSlotWidget> SaveSlotWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Save UI|Widget Classes")
	TSubclassOf<ULRCreateSaveSlotWidget> CreateSaveSlotWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Save UI|Widget Classes")
	TSubclassOf<ULRSaveConfirmDialogWidget> ConfirmDialogWidgetClass;

private:
	UFUNCTION()
	void HandleSaveSnapshotChanged(const FLRSaveUISnapshot& snapshot);
	UFUNCTION()
	void HandleSlotPrimaryRequested(FLRSaveSlotId slotId);
	UFUNCTION()
	void HandleSlotFocused(FLRSaveSlotId slotId);
	UFUNCTION()
	void HandleCreateSlotClicked();
	UFUNCTION()
	void HandleCreateSlotFocused();
	UFUNCTION()
	void HandleConfirmRequested();
	UFUNCTION()
	void HandleCancelRequested();
	UFUNCTION()
	void HandleBackClicked();

	void RebuildSlotRows(const FLRSaveUISnapshot& snapshot);
	void AddSlotRow(UWidget* row);
	void EnsureConfirmDialog();
	void UnbindSaveController();

	UPROPERTY(Transient) TMap<FLRSaveSlotId, TObjectPtr<ULRSaveSlotWidget>> SlotRows;
	UPROPERTY(Transient) TObjectPtr<ULRCreateSaveSlotWidget> CreateSlotRow;
	UPROPERTY(Transient) TObjectPtr<ULRSaveConfirmDialogWidget> ConfirmDialog;
};
