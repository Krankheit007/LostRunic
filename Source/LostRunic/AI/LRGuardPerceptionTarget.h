/**
 * @file LRGuardPerceptionTarget.h
 * @brief Defines the target-side capability queried by guard sight rules.
 */
#pragma once

#include "UObject/Interface.h"

#include "LRGuardPerceptionTarget.generated.h"

/** Blueprint-visible capability marker for actors that can be considered by guard sight. */
UINTERFACE(BlueprintType, meta = (DisplayName = "Lost Runic Guard Perception Target"))
class LOSTRUNIC_API ULRGuardPerceptionTarget : public UInterface
{
	GENERATED_BODY()
};

/** Target-side guard sight capability; visibility and concealment remain separate queries. */
class LOSTRUNIC_API ILRGuardPerceptionTarget
{
	GENERATED_BODY()

public:
	/** Returns whether this actor is a relevant target for guard sight selection. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Lost Runic|AI|Perception")
	bool IsRelevantGuardSightTarget() const;
};
