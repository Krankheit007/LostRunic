/** @file LRSaveAnchor.h @brief Stable world anchor used by save restoration. */
#pragma once

#include "GameFramework/Actor.h"

#include "LRSaveAnchor.generated.h"

UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Save Anchor"))
class LOSTRUNIC_API ALRSaveAnchor : public AActor
{
	GENERATED_BODY()

public:
	ALRSaveAnchor();
	static ALRSaveAnchor* FindById(const UWorld* world, FName anchorId);
	static ALRSaveAnchor* FindNearest(const UWorld* world, const FVector& location);
	static bool ValidateUniqueIds(const UWorld* world, FString& outError);

	UFUNCTION(BlueprintPure, Category = "Lost Runic|Save")
	FName GetAnchorId() const { return AnchorId; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Save", meta = (ToolTip = "Stable ID unique within this map."))
	FName AnchorId = NAME_None;
};
