#pragma once

#include "UObject/Interface.h"

#include "LRGuardVisibility.generated.h"

UINTERFACE(BlueprintType, meta = (DisplayName = "Lost Runic Guard Visibility"))
class LOSTRUNIC_API ULRGuardVisibility : public UInterface
{
	GENERATED_BODY()
};

/** Optional capability used by cover and hiding systems to suppress guard sight. */
class LOSTRUNIC_API ILRGuardVisibility
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Lost Runic|Stealth")
	bool IsVisibleToGuard(AActor* guard) const;
};
