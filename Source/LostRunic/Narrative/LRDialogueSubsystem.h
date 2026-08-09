#pragma once

#include "Narrative/LRNarrativeTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "LRDialogueSubsystem.generated.h"

class ULRGameContentSet;
class ULRLevelEventDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRNarrativePageChanged, FLRNarrativePage, page);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLRNarrativeSessionEnded, ELRNarrativeSessionType, sessionType,
	FName, finalContentId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLRNarrativeEventCommitted, FName, eventId,
	ELRSavePolicy, savePolicy);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLRNarrativeRequestRejected, FName, contentId,
	FGameplayTag, reason);

/** Owns DataTable dialogue traversal, narrative conditions, and stable one-shot events. */
UCLASS(meta = (DisplayName = "Lost Runic Dialogue Subsystem"))
class LOSTRUNIC_API ULRDialogueSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& collection) override;
	virtual void Deinitialize() override;

	void InitializeContent(ULRGameContentSet* contentSet);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Narrative")
	FLRNarrativeResult StartDialogue(FName rowId, FName completionEventId = NAME_None);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Narrative")
	FLRNarrativeResult StartReading(FName readingId, FName completionEventId = NAME_None);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Narrative")
	FLRNarrativeResult Advance();

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Narrative")
	FLRNarrativeResult SelectChoice(FName choiceId);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Narrative")
	void EndSession();

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Narrative|Events")
	FLRNarrativeResult TryCompleteEvent(FName eventId);

	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Narrative|Conditions")
	void SetContextTags(const FGameplayTagContainer& contextTags);

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Narrative|Conditions")
	FGameplayTagContainer GetContextTags() const { return ContextTags; }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Narrative")
	bool HasActiveSession() const { return CurrentPage.SessionType != ELRNarrativeSessionType::None; }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Narrative")
	FLRNarrativePage GetCurrentPage() const { return CurrentPage; }

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Narrative|Events")
	bool IsEventCompleted(FName eventId) const { return CompletedEventIds.Contains(eventId); }

	void RestoreCompletedEvents(const TSet<FName>& eventIds);
	const TSet<FName>& GetCompletedEvents() const { return CompletedEventIds; }

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Narrative")
	FLRNarrativePageChanged OnPageChanged;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Narrative")
	FLRNarrativeSessionEnded OnSessionEnded;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Narrative|Events")
	FLRNarrativeEventCommitted OnEventCommitted;

	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Narrative")
	FLRNarrativeRequestRejected OnRequestRejected;

private:
	FLRNarrativeResult ShowDialogueRow(FName rowId, ELRNarrativeAction action);
	FLRNarrativeResult ShowReadingRow(FName readingId);
	FLRNarrativeResult FinishSession();
	FLRNarrativeResult Reject(FName contentId, FGameplayTag reason);
	ULRLevelEventDefinition* FindEventDefinition(FName eventId) const;
	void ResetSession();

	UPROPERTY(Transient)
	TObjectPtr<ULRGameContentSet> ContentSet;

	UPROPERTY(Transient)
	FLRNarrativePage CurrentPage;

	UPROPERTY(Transient)
	FGameplayTagContainer ContextTags;

	UPROPERTY(Transient)
	TSet<FName> CompletedEventIds;

	FName CompletionEventId = NAME_None;
};
