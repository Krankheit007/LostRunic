#include "Save/LRSaveAnchor.h"

#include "Core/LRLog.h"
#include "EngineUtils.h"

ALRSaveAnchor::ALRSaveAnchor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ALRSaveAnchor::BeginPlay()
{
	Super::BeginPlay();
	FString error;
	ensureMsgf(ValidateUniqueIds(GetWorld(), error), TEXT("%s"), *error);
}

ALRSaveAnchor* ALRSaveAnchor::FindById(const UWorld* world, const FName anchorId)
{
	if (!world || anchorId.IsNone())
	{
		return nullptr;
	}
	for (TActorIterator<ALRSaveAnchor> iterator(world); iterator; ++iterator)
	{
		if (iterator->AnchorId == anchorId)
		{
			return *iterator;
		}
	}
	return nullptr;
}

ALRSaveAnchor* ALRSaveAnchor::FindNearest(const UWorld* world, const FVector& location)
{
	ALRSaveAnchor* nearest = nullptr;
	double nearestDistanceSquared = TNumericLimits<double>::Max();
	for (TActorIterator<ALRSaveAnchor> iterator(world); iterator; ++iterator)
	{
		const double distanceSquared = FVector::DistSquared(location, iterator->GetActorLocation());
		if (!iterator->AnchorId.IsNone() && distanceSquared < nearestDistanceSquared)
		{
			nearest = *iterator;
			nearestDistanceSquared = distanceSquared;
		}
	}
	return nearest;
}

bool ALRSaveAnchor::ValidateUniqueIds(const UWorld* world, FString& outError)
{
	TSet<FName> ids;
	for (TActorIterator<ALRSaveAnchor> iterator(world); iterator; ++iterator)
	{
		if (iterator->AnchorId.IsNone() || ids.Contains(iterator->AnchorId))
		{
			outError = FString::Printf(TEXT("World '%s' has an empty or duplicate SaveAnchor ID '%s'."),
				*GetNameSafe(world), *iterator->AnchorId.ToString());
			return false;
		}
		ids.Add(iterator->AnchorId);
	}
	return true;
}
