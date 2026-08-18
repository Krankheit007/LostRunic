/** @file LRDialogueStateParticipant.h @brief Supplies Story.* variables to SUDS. */
#pragma once

#include "CoreMinimal.h"
#include "SUDSParticipant.h"

#include "LRDialogueStateParticipant.generated.h"

class ULRStoryStateSubsystem;

UCLASS()
class LOSTRUNIC_API ULRDialogueStateParticipant : public UObject, public ISUDSParticipant
{
	GENERATED_BODY()

public:
	void Initialize(ULRStoryStateSubsystem* InStoryState) { StoryState = InStoryState; }

	virtual void OnDialogueVariableRequested_Implementation(USUDSDialogue* Dialogue, FName VariableName) override;
	virtual int GetDialogueParticipantPriority_Implementation() const override { return 1000; }

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<ULRStoryStateSubsystem> StoryState;
};
