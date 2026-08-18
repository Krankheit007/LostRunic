/** @file LRDialogueScriptRegistry.h @brief Stable ScriptId to SUDS script mapping. */
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "LRDialogueScriptRegistry.generated.h"

class USUDSScript;

USTRUCT(BlueprintType)
struct LOSTRUNIC_API FLRDialogueScriptDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue")
	FName ScriptId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue")
	TObjectPtr<USUDSScript> Script = nullptr;
};

UCLASS(BlueprintType)
class LOSTRUNIC_API ULRDialogueScriptRegistry : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue")
	TArray<FLRDialogueScriptDefinition> Scripts;

	const FLRDialogueScriptDefinition* FindDefinition(FName ScriptId) const;
	bool Resolve(FName ScriptId, TObjectPtr<USUDSScript>& OutScript, FString& OutError) const;
	bool Validate(FString& OutError) const;
};
