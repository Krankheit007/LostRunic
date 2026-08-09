#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "LRProjectSettings.generated.h"

class ULRGameContentSet;
class ULRGameTuningSet;
class ULRInputConfig;

/** Project-level asset roots loaded by the LostRunic game-instance subsystem. */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Lost Runic"))
class LOSTRUNIC_API ULRProjectSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Content")
	TSoftObjectPtr<ULRGameContentSet> ContentSet;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Tuning")
	TSoftObjectPtr<ULRGameTuningSet> TuningSet;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Input")
	TSoftObjectPtr<ULRInputConfig> InputConfig;

	virtual FName GetCategoryName() const override { return TEXT("Game"); }
};
