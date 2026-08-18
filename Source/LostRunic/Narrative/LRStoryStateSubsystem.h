/** @file LRStoryStateSubsystem.h @brief Persistent GameplayTag-backed story state. */
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "LRStoryStateSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRStoryFlagAdded, FGameplayTag, Flag);

UCLASS()
class LOSTRUNIC_API ULRStoryStateSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Lost Runic|Story")
	bool AddStoryFlag(FGameplayTag Flag);

	UFUNCTION(BlueprintPure, Category="Lost Runic|Story")
	bool HasStoryFlag(FGameplayTag Flag) const;

	UFUNCTION(BlueprintPure, Category="Lost Runic|Story")
	FGameplayTagContainer GetStoryFlags() const { return StoryFlags; }

	void CaptureSaveState(FGameplayTagContainer& OutFlags) const { OutFlags = StoryFlags; }
	bool RestoreSaveState(const FGameplayTagContainer& InFlags);

	UFUNCTION(BlueprintCallable, Category="Lost Runic|Story")
	void ResetForNewGame();

	UPROPERTY(BlueprintAssignable, Category="Lost Runic|Story")
	FLRStoryFlagAdded OnStoryFlagAdded;

private:
	UPROPERTY(Transient)
	FGameplayTagContainer StoryFlags;
};
