/** @file LRMainMenuWidget.h @brief 主菜单 Widget 父类与 BindWidget 契约。 */
#pragma once

#include "Blueprint/UserWidget.h"
#include "UI/LRMainMenuUITypes.h"

#include "LRMainMenuWidget.generated.h"

class UButton;
class ULRMainMenuWidgetController;

UCLASS(Abstract, Blueprintable, meta = (DisplayName = "Lost Runic Main Menu Widget"))
class LOSTRUNIC_API ULRMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetMainMenuWidgetController(ULRMainMenuWidgetController* controller);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Lost Runic|Main Menu")
	void OnMainMenuViewChanged(const FLRMainMenuViewModel& viewModel);

	UFUNCTION(BlueprintImplementableEvent, Category = "Lost Runic|Main Menu")
	void OnLoadRequested();

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UButton> NewGameButton;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UButton> ContinueButton;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UButton> LoadButton;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UButton> OptionsButton;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UButton> ExitButton;

private:
	UFUNCTION()
	void HandleViewChanged(const FLRMainMenuViewModel& viewModel);
	UFUNCTION()
	void HandleNewGameClicked();
	UFUNCTION()
	void HandleContinueClicked();
	UFUNCTION()
	void HandleLoadClicked();
	UFUNCTION()
	void HandleExitClicked();

	UPROPERTY(Transient)
	TObjectPtr<ULRMainMenuWidgetController> MainMenuWidgetController;
};
