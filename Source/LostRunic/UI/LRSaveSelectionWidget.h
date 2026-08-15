/** @file LRSaveSelectionWidget.h @brief 存档选择页面父类：绑定控制器快照，不拥有控制器生命周期。 */
#pragma once

#include "UI/LRScreenWidget.h"
#include "UI/LRSaveUITypes.h"

#include "LRSaveSelectionWidget.generated.h"

class UButton;
class UPanelWidget;
class UTextBlock;

UCLASS(Abstract, Blueprintable, meta = (DisplayName = "Lost Runic Save Selection Widget"))
class LOSTRUNIC_API ULRSaveSelectionWidget : public ULRScreenWidget
{
	GENERATED_BODY()

public:
	virtual void SetSaveWidgetController(ULRSaveWidgetController* controller) override;
	virtual bool HandleUICommand_Implementation(ELRUICommand command) override;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Lost Runic|Save UI")
	void OnSaveSnapshotChanged(const FLRSaveUISnapshot& snapshot);

	UFUNCTION(BlueprintImplementableEvent, Category = "Lost Runic|Save UI")
	void OnBackRequested();

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UPanelWidget> SlotListPanel;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UButton> CreateSlotButton;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UButton> BackButton;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UTextBlock> StatusText;

private:
	UFUNCTION()
	void HandleSaveSnapshotChanged(const FLRSaveUISnapshot& snapshot);
	UFUNCTION()
	void HandleCreateSlotClicked();
	UFUNCTION()
	void HandleBackClicked();

	void UnbindSaveController();
};
