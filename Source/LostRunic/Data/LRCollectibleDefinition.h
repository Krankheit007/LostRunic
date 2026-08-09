#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "LRCollectibleDefinition.generated.h"

class UStaticMesh;
class UTexture2D;

/** Immutable collectible content referenced by a stable CollectibleId. */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Collectible Definition"))
class LOSTRUNIC_API ULRCollectibleDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Collectible")
	FName CollectibleId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Collectible")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Collectible", meta = (MultiLine = "true"))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Collectible")
	TSoftObjectPtr<UStaticMesh> DisplayMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Collectible")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Collectible", meta = (ClampMin = "0"))
	int32 DisplayOrder = 0;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif
};
