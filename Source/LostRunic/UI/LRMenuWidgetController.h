#pragma once

#include "UI/LRUITypes.h"
#include "UObject/Object.h"

#include "LRMenuWidgetController.generated.h"

class ULRInventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLRMenuScreenChanged, ELRScreenType, previousScreen, ELRScreenType, currentScreen);

/** Owns menu navigation state and prepares immutable inventory/journal snapshots for layouts. */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Menu Widget Controller"))
class LOSTRUNIC_API ULRMenuWidgetController : public UObject
{
	GENERATED_BODY()

public:
	bool OpenScreen(ELRScreenType screen);
	void CloseScreen();
	FLRInventorySnapshot BuildInventorySnapshot(const ULRInventoryComponent* inventory) const;

	ELRScreenType GetOpenScreen() const { return OpenScreenType; }

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|UI")
	FLRMenuScreenChanged OnMenuScreenChanged;

private:
	ELRScreenType OpenScreenType = ELRScreenType::None;
};
