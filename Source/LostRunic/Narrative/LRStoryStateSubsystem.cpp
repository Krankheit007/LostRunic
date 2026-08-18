/** @file LRStoryStateSubsystem.cpp @brief Persistent GameplayTag-backed story state. */
#include "Narrative/LRStoryStateSubsystem.h"

#include "Core/LRLog.h"

bool ULRStoryStateSubsystem::AddStoryFlag(const FGameplayTag Flag)
{
	if (!Flag.IsValid() || !Flag.ToString().StartsWith(TEXT("Story.")))
	{
		UE_LOG(LogLostRunicNarrative, Warning, TEXT("Rejected non-Story flag=%s."), *Flag.ToString());
		return false;
	}
	if (StoryFlags.HasTagExact(Flag))
	{
		return false;
	}
	StoryFlags.AddTag(Flag);
	OnStoryFlagAdded.Broadcast(Flag);
	return true;
}

bool ULRStoryStateSubsystem::HasStoryFlag(const FGameplayTag Flag) const
{
	return Flag.IsValid() && StoryFlags.HasTag(Flag);
}

bool ULRStoryStateSubsystem::RestoreSaveState(const FGameplayTagContainer& InFlags)
{
	for (const FGameplayTag& Flag : InFlags)
	{
		if (!Flag.IsValid() || !Flag.ToString().StartsWith(TEXT("Story.")))
		{
			UE_LOG(LogLostRunicNarrative, Warning, TEXT("Save contained invalid StoryFlag=%s."), *Flag.ToString());
			return false;
		}
	}
	StoryFlags = InFlags;
	return true;
}

void ULRStoryStateSubsystem::ResetForNewGame()
{
	StoryFlags.Reset();
}
