/** @file LRDialogueEventBridge.h @brief Dispatches SUDS event namespaces into LostRunic systems. */
#pragma once

#include "CoreMinimal.h"
#include "Narrative/LRStoryStateSubsystem.h"
#include "UObject/Object.h"

#include "LRDialogueEventBridge.generated.h"

class USUDSDialogue;
struct FSUDSValue;

UCLASS()
class LOSTRUNIC_API ULRDialogueEventBridge : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(ULRStoryStateSubsystem* InStoryState) { StoryState = InStoryState; }

	UFUNCTION()
	void HandleDialogueEvent(USUDSDialogue* Dialogue, FName EventName, const TArray<FSUDSValue>& Arguments);

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<ULRStoryStateSubsystem> StoryState;
};
