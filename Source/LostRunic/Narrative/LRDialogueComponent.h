/** @file LRDialogueComponent.h @brief Actor-owned SUDS dialogue entry point. */
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"

#include "LRDialogueComponent.generated.h"

class ULRDialogueScriptRegistry;

UCLASS(ClassGroup=(LostRunic), meta=(BlueprintSpawnableComponent))
class LOSTRUNIC_API ULRDialogueComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Lost Runic|Dialogue")
	bool TryStartDialogue(AActor* Instigator);

	UFUNCTION(BlueprintCallable, Category="Lost Runic|Dialogue")
	void EndDialogue();

	UFUNCTION(BlueprintPure, Category="Lost Runic|Dialogue")
	FName GetScriptId() const { return ScriptId; }

	/** Validates this component's local ScriptId against the current global Registry. */
	bool Validate(const ULRDialogueScriptRegistry* CurrentRegistry, FString& OutError) const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Dialogue")
	FName ScriptId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Dialogue")
	FName StartLabel = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Dialogue")
	FGameplayTag CompletionStoryTag;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
