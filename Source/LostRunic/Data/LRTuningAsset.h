#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "LRTuningAsset.generated.h"

/** Base class that exposes identical runtime and editor validation behavior. */
UCLASS(Abstract, BlueprintType, meta = (DisplayName = "Lost Runic Tuning Asset"))
class LOSTRUNIC_API ULRTuningAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	virtual bool Validate(FString& outError) const PURE_VIRTUAL(ULRTuningAsset::Validate, return false;);

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif
};
