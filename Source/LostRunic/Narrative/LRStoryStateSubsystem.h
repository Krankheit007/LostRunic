/** @file LRStoryStateSubsystem.h @brief Persistent GameplayTag-backed story state. */
#pragma once

#include "Narrative/LRNarrativeTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "LRStoryStateSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRStoryFlagAdded, FGameplayTag, Flag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRStoryEventCommitted, FLRStoryEventCommit, Event);
DECLARE_MULTICAST_DELEGATE_OneParam(FLRStoryEventCommittedNative, const FLRStoryEventCommit&);

class UGameInstance;

UCLASS()
class LOSTRUNIC_API ULRStoryStateSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static ULRStoryStateSubsystem* Resolve(UGameInstance* gameInstance);

	UFUNCTION(BlueprintCallable, Category="Lost Runic|Story")
	bool AddStoryFlag(FGameplayTag Flag);

	UFUNCTION(BlueprintPure, Category="Lost Runic|Story")
	bool HasStoryFlag(FGameplayTag Flag) const;

	UFUNCTION(BlueprintPure, Category="Lost Runic|Story")
	FGameplayTagContainer GetStoryFlags() const { return PersistentState.StoryFlags; }

	UFUNCTION(BlueprintPure, Category="Lost Runic|Story")
	bool IsEventCompleted(FName eventId) const;

	UFUNCTION(BlueprintPure, Category="Lost Runic|Story")
	bool HasMemoryEvent(FName eventId) const;

	void CapturePersistentState(FLRNarrativePersistentState& outState) const { outState = PersistentState; }
	bool ReplacePersistentState(const FLRNarrativePersistentState& inState);
	bool ApplyPersistentDelta(const FLRNarrativePersistentDelta& inDelta);
	bool CommitEvent(const FLRStoryEventCommit& eventCommit);
	bool CommitMemoryEvent(FName eventId);

	const TSet<FName>& GetCompletedEventIds() const { return PersistentState.CompletedEventIds; }
	const TSet<FName>& GetMemoryEventIds() const { return PersistentState.MemoryEventIds; }

	UFUNCTION(BlueprintCallable, Category="Lost Runic|Story")
	void ResetForNewGame();

	UPROPERTY(BlueprintAssignable, Category="Lost Runic|Story")
	FLRStoryFlagAdded OnStoryFlagAdded;

	UPROPERTY(BlueprintAssignable, Category="Lost Runic|Story")
	FLRStoryEventCommitted OnStoryEventCommitted;

	FLRStoryEventCommittedNative OnStoryEventCommittedNative;

private:
	UPROPERTY(Transient)
	FLRNarrativePersistentState PersistentState;
};
