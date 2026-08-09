#pragma once

#include "Blueprint/UserWidget.h"
#include "UI/LRUITypes.h"

#include "LRScreenWidget.generated.h"

/** Reusable screen base; Blueprint layouts receive presentation changes but never own gameplay rules. */
UCLASS(Abstract, Blueprintable, meta = (DisplayName = "Lost Runic Screen Widget"))
class LOSTRUNIC_API ULRScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetScreenVisible(bool bVisible);
	void PresentNarrative(const FLRNarrativePresentation& presentation);

	ELRScreenType GetScreenType() const { return ScreenType; }
	bool IsScreenVisible() const { return bScreenVisible; }

	UFUNCTION(BlueprintImplementableEvent, Category = "Lost Runic|UI")
	void OnScreenVisibilityChanged(bool bVisible);

	UFUNCTION(BlueprintImplementableEvent, Category = "Lost Runic|UI")
	void OnNarrativePresentationChanged(const FLRNarrativePresentation& presentation);

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Screen")
	ELRScreenType ScreenType = ELRScreenType::None;

private:
	bool bScreenVisible = false;
};
