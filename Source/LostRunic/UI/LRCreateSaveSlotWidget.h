/** @file LRCreateSaveSlotWidget.h @brief 创建手动存档槽 Widget 父类。 */
#pragma once

#include "Blueprint/UserWidget.h"

#include "LRCreateSaveSlotWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLRCreateSaveSlotRequested);

UCLASS(Abstract, Blueprintable, meta = (DisplayName = "Lost Runic Create Save Slot Widget"))
class LOSTRUNIC_API ULRCreateSaveSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnAddedToFocusPath(const FFocusEvent& inFocusEvent) override;
	void ApplyView(int32 displayIndex, const FText& label);
	bool SetCreateFocus();

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Save UI")
	FLRCreateSaveSlotRequested OnCreateRequested;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Save UI")
	FLRCreateSaveSlotRequested OnFocused;

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UButton> CreateButton;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UTextBlock> Slot_Index;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UTextBlock> CreateLabelText;

private:
	UFUNCTION()
	void HandleCreateClicked();
};
