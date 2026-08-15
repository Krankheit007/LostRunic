/** @file LRMainMenuHUD.h @brief 主菜单 Host HUD；沿用 PlayerController，但不创建玩法 Pawn。 */
#pragma once

#include "UI/LRHUD.h"
#include "UI/LRMainMenuWidgetController.h"

#include "LRMainMenuHUD.generated.h"

UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Main Menu HUD"))
class LOSTRUNIC_API ALRMainMenuHUD : public ALRHUD
{
	GENERATED_BODY()

public:
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
	virtual void InitializeForController(ALRPlayerController* playerController) override;

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Main Menu")
	ULRMainMenuWidgetController* GetMainMenuWidgetController() const { return MainMenuWidgetController; }

private:
	UPROPERTY(Transient)
	TObjectPtr<ULRMainMenuWidgetController> MainMenuWidgetController;
};
