#include "Narrative/LRDialogueSubsystem.h"

#include "Core/LRGameplayTags.h"
#include "Core/LRLog.h"
#include "Data/LRContentRows.h"
#include "Data/LRGameContentSet.h"
#include "Data/LRLevelEventDefinition.h"
#include "Engine/DataTable.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "Narrative/LRNarrativeRules.h"

void ULRDialogueSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
	Super::Initialize(collection);
	collection.InitializeDependency<ULRGameInstanceSubsystem>();
	const ULRGameInstanceSubsystem* dataSubsystem = GetGameInstance()->GetSubsystem<ULRGameInstanceSubsystem>();
	InitializeContent(dataSubsystem ? dataSubsystem->GetContentSet() : nullptr);
}

void ULRDialogueSubsystem::Deinitialize()
{
	ResetSession();
	ContentSet = nullptr;
	ContextTags.Reset();
	CompletedEventIds.Reset();
	Super::Deinitialize();
}

void ULRDialogueSubsystem::InitializeContent(ULRGameContentSet* contentSet)
{
	ContentSet = contentSet;
	ResetSession();
}

FLRNarrativeResult ULRDialogueSubsystem::StartDialogue(const FName rowId, const FName completionEventId)
{
	ResetSession();
	CompletionEventId = completionEventId;
	FLRNarrativeResult result = ShowDialogueRow(rowId, ELRNarrativeAction::Started);
	if (!result.bSuccess)
	{
		ResetSession();
	}
	return result;
}

FLRNarrativeResult ULRDialogueSubsystem::StartReading(const FName readingId, const FName completionEventId)
{
	ResetSession();
	CompletionEventId = completionEventId;
	FLRNarrativeResult result = ShowReadingRow(readingId);
	if (!result.bSuccess)
	{
		ResetSession();
	}
	return result;
}

FLRNarrativeResult ULRDialogueSubsystem::Advance()
{
	if (!HasActiveSession())
	{
		return Reject(NAME_None, LRGameplayTags::NarrativeRejectNoSession);
	}
	if (CurrentPage.SessionType == ELRNarrativeSessionType::Reading)
	{
		return FinishSession();
	}
	if (!CurrentPage.Choices.IsEmpty())
	{
		FLRNarrativeResult result;
		result.bSuccess = true;
		result.Action = ELRNarrativeAction::AwaitChoice;
		result.ContentId = CurrentPage.ContentId;
		return result;
	}

	const FLRDialogueRow* row = ContentSet && ContentSet->DialogueTable
		? ContentSet->DialogueTable->FindRow<FLRDialogueRow>(CurrentPage.ContentId, TEXT("Advance dialogue")) : nullptr;
	if (!row)
	{
		return Reject(CurrentPage.ContentId, LRGameplayTags::NarrativeRejectMissingContent);
	}
	if (!row->Options.IsEmpty())
	{
		return Reject(CurrentPage.ContentId, LRGameplayTags::NarrativeRejectConditions);
	}
	return row->NextRowId.IsNone() ? FinishSession()
		: ShowDialogueRow(row->NextRowId, ELRNarrativeAction::Advanced);
}

FLRNarrativeResult ULRDialogueSubsystem::SelectChoice(const FName choiceId)
{
	if (CurrentPage.SessionType != ELRNarrativeSessionType::Dialogue)
	{
		return Reject(CurrentPage.ContentId, LRGameplayTags::NarrativeRejectNoSession);
	}
	const FLRNarrativeChoice* choice = CurrentPage.Choices.FindByPredicate([choiceId](const FLRNarrativeChoice& candidate)
	{
		return candidate.ChoiceId == choiceId;
	});
	if (!choice)
	{
		return Reject(CurrentPage.ContentId, LRGameplayTags::NarrativeRejectInvalidChoice);
	}
	return choice->NextContentId.IsNone() ? FinishSession()
		: ShowDialogueRow(choice->NextContentId, ELRNarrativeAction::Advanced);
}

void ULRDialogueSubsystem::EndSession()
{
	if (!HasActiveSession())
	{
		return;
	}
	const ELRNarrativeSessionType sessionType = CurrentPage.SessionType;
	const FName finalContentId = CurrentPage.ContentId;
	ResetSession();
	OnSessionEnded.Broadcast(sessionType, finalContentId);
}

FLRNarrativeResult ULRDialogueSubsystem::ShowDialogueRow(const FName rowId, const ELRNarrativeAction action)
{
	const FLRDialogueRow* row = ContentSet && ContentSet->DialogueTable
		? ContentSet->DialogueTable->FindRow<FLRDialogueRow>(rowId, TEXT("Show dialogue")) : nullptr;
	if (!row)
	{
		return Reject(rowId, LRGameplayTags::NarrativeRejectMissingContent);
	}
	if (!LRNarrativeRules::AreConditionsMet(row->RequiredTags, row->BlockedTags, ContextTags))
	{
		return Reject(rowId, LRGameplayTags::NarrativeRejectConditions);
	}
	if (row->DialogueId != rowId)
	{
		UE_LOG(LogLostRunicNarrative, Warning, TEXT("Dialogue row name=%s has mismatched ID=%s"),
			*rowId.ToString(), *row->DialogueId.ToString());
		return Reject(rowId, LRGameplayTags::NarrativeRejectMissingContent);
	}

	CurrentPage = FLRNarrativePage();
	CurrentPage.SessionType = ELRNarrativeSessionType::Dialogue;
	CurrentPage.ContentId = row->DialogueId;
	CurrentPage.SpeakerId = row->SpeakerId;
	CurrentPage.Text = row->Text;
	CurrentPage.Portrait = row->Portrait;
	LRNarrativeRules::BuildAvailableChoices(row->Options, ContextTags, CurrentPage.Choices);
	OnPageChanged.Broadcast(CurrentPage);

	FLRNarrativeResult result;
	result.bSuccess = true;
	result.Action = action;
	result.ContentId = CurrentPage.ContentId;
	return result;
}

FLRNarrativeResult ULRDialogueSubsystem::ShowReadingRow(const FName readingId)
{
	const FLRReadingRow* row = ContentSet && ContentSet->ReadingTable
		? ContentSet->ReadingTable->FindRow<FLRReadingRow>(readingId, TEXT("Show reading")) : nullptr;
	if (!row)
	{
		return Reject(readingId, LRGameplayTags::NarrativeRejectMissingContent);
	}
	if (row->ReadingId != readingId)
	{
		UE_LOG(LogLostRunicNarrative, Warning, TEXT("Reading row name=%s has mismatched ID=%s"),
			*readingId.ToString(), *row->ReadingId.ToString());
		return Reject(readingId, LRGameplayTags::NarrativeRejectMissingContent);
	}

	CurrentPage = FLRNarrativePage();
	CurrentPage.SessionType = ELRNarrativeSessionType::Reading;
	CurrentPage.ContentId = row->ReadingId;
	CurrentPage.Title = row->Title;
	CurrentPage.Text = row->Body;
	OnPageChanged.Broadcast(CurrentPage);

	FLRNarrativeResult result;
	result.bSuccess = true;
	result.Action = ELRNarrativeAction::Started;
	result.ContentId = CurrentPage.ContentId;
	return result;
}

FLRNarrativeResult ULRDialogueSubsystem::FinishSession()
{
	const FName eventId = CompletionEventId;
	const ELRNarrativeSessionType sessionType = CurrentPage.SessionType;
	const FName finalContentId = CurrentPage.ContentId;
	ResetSession();
	OnSessionEnded.Broadcast(sessionType, finalContentId);

	if (!eventId.IsNone())
	{
		FLRNarrativeResult eventResult = TryCompleteEvent(eventId);
		if (!eventResult.bSuccess)
		{
			return eventResult;
		}
	}
	FLRNarrativeResult result;
	result.bSuccess = true;
	result.Action = ELRNarrativeAction::Completed;
	result.ContentId = finalContentId;
	return result;
}

FLRNarrativeResult ULRDialogueSubsystem::Reject(const FName contentId, const FGameplayTag reason)
{
	FLRNarrativeResult result;
	result.ContentId = contentId;
	result.FailureReason = reason;
	OnRequestRejected.Broadcast(contentId, reason);
	UE_LOG(LogLostRunicNarrative, Verbose, TEXT("NarrativeContent=%s rejected reason=%s"),
		*contentId.ToString(), *reason.ToString());
	return result;
}

void ULRDialogueSubsystem::ResetSession()
{
	CurrentPage = FLRNarrativePage();
	CompletionEventId = NAME_None;
}
