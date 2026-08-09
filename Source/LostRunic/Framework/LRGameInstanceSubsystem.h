#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "LRGameInstanceSubsystem.generated.h"

class ULRGameContentSet;
class ULRGameTuningSet;

/** Owns validated project content roots for the lifetime of a game instance. */
UCLASS(meta = (DisplayName = "Lost Runic Game Instance Subsystem"))
class LOSTRUNIC_API ULRGameInstanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Data")
	ULRGameTuningSet* GetTuningSet() const { return TuningSet; }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Data")
	ULRGameContentSet* GetContentSet() const { return ContentSet; }

	bool HasValidConfiguration() const { return bConfigurationValid; }

private:
	UPROPERTY(Transient)
	TObjectPtr<ULRGameTuningSet> TuningSet;

	UPROPERTY(Transient)
	TObjectPtr<ULRGameContentSet> ContentSet;

	bool bConfigurationValid = false;
};
