/** @file LRPauseWidget.h @brief Designer-authored pause menu binding contract. */
#pragma once

#include "UI/LRScreenWidget.h"

#include "LRPauseWidget.generated.h"

class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLRPauseActionRequested);

UCLASS(Abstract, Blueprintable, meta = (DisplayName = "Lost Runic Pause Widget"))
class LOSTRUNIC_API ULRPauseWidget : public ULRScreenWidget
{
	GENERATED_BODY()

public:
	virtual bool HandleUICommand_Implementation(ELRUICommand command) override;
	virtual bool SetInitialFocus() override;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Pause")
	FLRPauseActionRequested OnResumeRequested;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Pause")
	FLRPauseActionRequested OnSaveRequested;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Pause")
	FLRPauseActionRequested OnMainMenuRequested;

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UButton> Resume;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UButton> SaveGame;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UButton> Options;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UButton> MainMenu;

private:
	UFUNCTION() void HandleResumeClicked();
	UFUNCTION() void HandleSaveClicked();
	UFUNCTION() void HandleMainMenuClicked();
	bool ExecuteFocusedAction();
};
