/** @file LRStoryStateSubsystem.cpp @brief Persistent GameplayTag-backed story state. */
#include "Narrative/LRStoryStateSubsystem.h"

#include "Core/LRLog.h"
#include "Engine/GameInstance.h"
#include "UObject/UObjectHash.h"

namespace
{
	bool IsValidStoryFlag(const FGameplayTag& flag)
	{
		return flag.IsValid() && flag.ToString().StartsWith(TEXT("Story."));
	}

	bool IsValidPersistentEventId(const FName eventId)
	{
		return !eventId.IsNone();
	}

	bool ValidateStoryFlags(const FGameplayTagContainer& storyFlags)
	{
		for (const FGameplayTag& flag : storyFlags)
		{
			if (!IsValidStoryFlag(flag))
			{
				return false;
			}
		}
		return true;
	}

	bool ValidatePersistentEventIds(const TSet<FName>& eventIds)
	{
		for (const FName eventId : eventIds)
		{
			if (!IsValidPersistentEventId(eventId))
			{
				return false;
			}
		}
		return true;
	}

	bool ValidatePersistentState(const FLRNarrativePersistentState& persistentState)
	{
		return ValidateStoryFlags(persistentState.StoryFlags)
			&& ValidatePersistentEventIds(persistentState.CompletedEventIds)
			&& ValidatePersistentEventIds(persistentState.MemoryEventIds);
	}
}

ULRStoryStateSubsystem* ULRStoryStateSubsystem::Resolve(UGameInstance* gameInstance)
{
	if (!gameInstance)
	{
		return nullptr;
	}
	if (ULRStoryStateSubsystem* subsystem = gameInstance->GetSubsystem<ULRStoryStateSubsystem>())
	{
		return subsystem;
	}

	TArray<UObject*> ownedObjects;
	GetObjectsWithOuter(gameInstance, ownedObjects, EGetObjectsFlags::None);
	for (UObject* ownedObject : ownedObjects)
	{
		if (ULRStoryStateSubsystem* fallback = Cast<ULRStoryStateSubsystem>(ownedObject))
		{
			return fallback;
		}
	}
	return nullptr;
}

bool ULRStoryStateSubsystem::AddStoryFlag(const FGameplayTag Flag)
{
	if (!IsValidStoryFlag(Flag))
	{
		UE_LOG(LogLostRunicNarrative, Warning, TEXT("Rejected non-Story flag=%s."), *Flag.ToString());
		return false;
	}
	if (PersistentState.StoryFlags.HasTagExact(Flag))
	{
		return false;
	}
	PersistentState.StoryFlags.AddTag(Flag);
	OnStoryFlagAdded.Broadcast(Flag);
	return true;
}

bool ULRStoryStateSubsystem::HasStoryFlag(const FGameplayTag Flag) const
{
	return Flag.IsValid() && PersistentState.StoryFlags.HasTag(Flag);
}

bool ULRStoryStateSubsystem::IsEventCompleted(const FName eventId) const
{
	return IsValidPersistentEventId(eventId) && PersistentState.CompletedEventIds.Contains(eventId);
}

bool ULRStoryStateSubsystem::HasMemoryEvent(const FName eventId) const
{
	return IsValidPersistentEventId(eventId) && PersistentState.MemoryEventIds.Contains(eventId);
}

bool ULRStoryStateSubsystem::ReplacePersistentState(const FLRNarrativePersistentState& inState)
{
	if (!ValidatePersistentState(inState))
	{
		UE_LOG(LogLostRunicNarrative, Warning, TEXT("Rejected invalid persistent story state replacement."));
		return false;
	}
	PersistentState = inState;
	return true;
}

bool ULRStoryStateSubsystem::ApplyPersistentDelta(const FLRNarrativePersistentDelta& inDelta)
{
	FLRNarrativePersistentState deltaAsState;
	deltaAsState.StoryFlags = inDelta.AddedStoryFlags;
	deltaAsState.CompletedEventIds = inDelta.AddedCompletedEventIds;
	deltaAsState.MemoryEventIds = inDelta.AddedMemoryEventIds;
	if (!ValidatePersistentState(deltaAsState))
	{
		UE_LOG(LogLostRunicNarrative, Warning, TEXT("Rejected invalid persistent story delta."));
		return false;
	}

	for (const FGameplayTag& flag : inDelta.AddedStoryFlags)
	{
		PersistentState.StoryFlags.AddTag(flag);
	}
	PersistentState.CompletedEventIds.Append(inDelta.AddedCompletedEventIds);
	PersistentState.MemoryEventIds.Append(inDelta.AddedMemoryEventIds);
	return true;
}

bool ULRStoryStateSubsystem::CommitEvent(const FLRStoryEventCommit& eventCommit)
{
	if (!IsValidPersistentEventId(eventCommit.EventId))
	{
		UE_LOG(LogLostRunicNarrative, Warning, TEXT("Rejected Story event with invalid stable ID."));
		return false;
	}
	if (eventCommit.StoryFlag.IsValid() && !IsValidStoryFlag(eventCommit.StoryFlag))
	{
		UE_LOG(LogLostRunicNarrative, Warning, TEXT("Rejected Story event=%s with invalid Story flag=%s."),
			*eventCommit.EventId.ToString(), *eventCommit.StoryFlag.ToString());
		return false;
	}
	if (PersistentState.CompletedEventIds.Contains(eventCommit.EventId))
	{
		return false;
	}

	const bool bStoryFlagAdded = eventCommit.StoryFlag.IsValid()
		&& !PersistentState.StoryFlags.HasTag(eventCommit.StoryFlag);
	PersistentState.CompletedEventIds.Add(eventCommit.EventId);
	if (eventCommit.StoryFlag.IsValid())
	{
		PersistentState.StoryFlags.AddTag(eventCommit.StoryFlag);
	}
	if (bStoryFlagAdded)
	{
		OnStoryFlagAdded.Broadcast(eventCommit.StoryFlag);
	}
	OnStoryEventCommittedNative.Broadcast(eventCommit);
	OnStoryEventCommitted.Broadcast(eventCommit);
	return true;
}
bool ULRStoryStateSubsystem::CommitMemoryEvent(const FName eventId, FLRNarrativePersistentDelta* outDelta)
{
	if (!IsValidPersistentEventId(eventId) || PersistentState.MemoryEventIds.Contains(eventId))
	{
		return false;
	}

	PersistentState.MemoryEventIds.Add(eventId);
	if (outDelta)
	{
		outDelta->AddedMemoryEventIds.Add(eventId);
	}
	return true;
}

void ULRStoryStateSubsystem::ResetForNewGame()
{
	PersistentState = FLRNarrativePersistentState();
}
