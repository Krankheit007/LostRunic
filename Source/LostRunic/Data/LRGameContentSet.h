#pragma once

#include "CoreMinimal.h"
#include "Data/LRContentRows.h"
#include "Engine/DataAsset.h"

#include "LRGameContentSet.generated.h"

class UDataTable;
class ULRCollectibleDefinition;
class ULRGuardDefinition;
class ULRItemDefinition;
class ULRLevelEventDefinition;

/** Registry for LostRunic tables, maps, and immutable content definitions. */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Game Content Set"))
class LOSTRUNIC_API ULRGameContentSet : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Content|Tables")
	TObjectPtr<UDataTable> DialogueTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Content|Tables")
	TObjectPtr<UDataTable> ReadingTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Content|Definitions")
	TArray<TObjectPtr<ULRItemDefinition>> Items;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Content|Definitions")
	TArray<TObjectPtr<ULRCollectibleDefinition>> Collectibles;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Content|Definitions")
	TArray<TObjectPtr<ULRGuardDefinition>> Guards;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Content|Definitions")
	TArray<TObjectPtr<ULRLevelEventDefinition>> LevelEvents;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Content|Maps")
	TArray<FLRMapRegistration> Maps;

	bool Validate(FString& outError) const;

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Content")
	TSoftObjectPtr<UWorld> FindMap(FName mapId) const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif
};
