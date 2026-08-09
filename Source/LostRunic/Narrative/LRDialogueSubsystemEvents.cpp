#include "Narrative/LRDialogueSubsystem.h"

#include "Core/LRGameplayTags.h"
#include "Core/LRLog.h"
#include "Data/LRGameContentSet.h"
#include "Data/LRLevelEventDefinition.h"
#include "Narrative/LRNarrativeRules.h"

FLRNarrativeResult ULRDialogueSubsystem::TryCompleteEvent(const FName eventId)
{
	ULRLevelEventDefinition* definition = FindEventDefinition(eventId);
	if (!definition)
	{
		return Reject(eventId, LRGameplayTags::NarrativeRejectMissingContent);
	}
	if (!LRNarrativeRules::AreConditionsMet(definition->RequiredTags, definition->BlockedTags, ContextTags))
	{
		return Reject(eventId, LRGameplayTags::NarrativeRejectConditions);
	}
	if (definition->bOneShot && CompletedEventIds.Contains(eventId))
	{
		return Reject(eventId, LRGameplayTags::NarrativeRejectAlreadyCompleted);
	}

	CompletedEventIds.Add(eventId);
	OnEventCommitted.Broadcast(eventId, definition->SavePolicy);
	UE_LOG(LogLostRunicNarrative, Log, TEXT("NarrativeEvent=%s completed savePolicy=%d"),
		*eventId.ToString(), static_cast<int32>(definition->SavePolicy));
	FLRNarrativeResult result;
	result.bSuccess = true;
	result.Action = ELRNarrativeAction::Completed;
	result.ContentId = eventId;
	return result;
}

void ULRDialogueSubsystem::SetContextTags(const FGameplayTagContainer& contextTags)
{
	ContextTags = contextTags;
}

void ULRDialogueSubsystem::RestoreCompletedEvents(const TSet<FName>& eventIds)
{
	CompletedEventIds = eventIds;
}

ULRLevelEventDefinition* ULRDialogueSubsystem::FindEventDefinition(const FName eventId) const
{
	if (!ContentSet || eventId.IsNone())
	{
		return nullptr;
	}
	const TObjectPtr<ULRLevelEventDefinition>* found = ContentSet->LevelEvents.FindByPredicate(
		[eventId](const ULRLevelEventDefinition* definition)
		{
			return definition && definition->EventId == eventId;
		});
	return found ? found->Get() : nullptr;
}
