// Copyright LostRunic. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "LRDialogueSpeakerRegistry.generated.h"

class UTexture2D;

USTRUCT(BlueprintType)
struct LOSTRUNIC_API FLRDialogueSpeakerDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Speaker")
	FName SpeakerId = NAME_None;

	/** Must reference ST_DialogueSpeakers; ordinary transient FText is rejected by Validate. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Speaker")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Speaker")
	TObjectPtr<UTexture2D> Portrait = nullptr;
};

UCLASS(BlueprintType)
class LOSTRUNIC_API ULRDialogueSpeakerRegistry : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Speaker")
	TArray<FLRDialogueSpeakerDefinition> Speakers;

	const FLRDialogueSpeakerDefinition* Find(FName SpeakerId) const;
	bool Validate(FString& OutError) const;
};
