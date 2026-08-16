/** @file LRSaveSlotWidget.h @brief 单个存档槽 Widget 父类与绑定契约。 */
#pragma once

#include "Blueprint/UserWidget.h"
#include "UI/LRSaveUITypes.h"

#include "LRSaveSlotWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRSaveSlotActionRequested, FLRSaveSlotId, slotId);

UCLASS(Abstract, Blueprintable, meta = (DisplayName = "Lost Runic Save Slot Widget"))
class LOSTRUNIC_API ULRSaveSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnAddedToFocusPath(const FFocusEvent& inFocusEvent) override;

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Save UI")
	void ApplyView(const FLRSaveSlotView& view);

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Save UI")
	const FLRSaveSlotView& GetView() const { return View; }
	bool SetSlotFocus();

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Save UI")
	FLRSaveSlotActionRequested OnPrimaryRequested;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Save UI")
	FLRSaveSlotActionRequested OnFocused;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Lost Runic|Save UI")
	void OnViewChanged(const FLRSaveSlotView& view);

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UButton> SlotButton;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UTextBlock> SlotNameText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UTextBlock> MapNameText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UTextBlock> SavedAtText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UTextBlock> PlayTimeText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UTextBlock> CollectibleCountText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UTextBlock> HealthText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UImage> BackgroundImage;

	UPROPERTY(BlueprintReadOnly, Category = "Save UI")
	FLRSaveSlotView View;

	UFUNCTION()
	void HandlePrimaryClicked();
};
