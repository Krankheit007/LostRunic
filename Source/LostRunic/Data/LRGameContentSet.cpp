#include "Data/LRGameContentSet.h"

#include "Engine/DataTable.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

bool ULRGameContentSet::Validate(FString& outError) const
{
	if (!DialogueTable || DialogueTable->GetRowStruct() != FLRDialogueRow::StaticStruct())
	{
		outError = TEXT("DialogueTable must use FLRDialogueRow.");
		return false;
	}

	if (!ReadingTable || ReadingTable->GetRowStruct() != FLRReadingRow::StaticStruct())
	{
		outError = TEXT("ReadingTable must use FLRReadingRow.");
		return false;
	}

	TSet<FName> mapIds;
	for (const FLRMapRegistration& map : Maps)
	{
		if (map.MapId.IsNone() || map.World.IsNull() || mapIds.Contains(map.MapId))
		{
			outError = FString::Printf(TEXT("Map registration '%s' is empty, missing its world, or duplicated."), *map.MapId.ToString());
			return false;
		}
		mapIds.Add(map.MapId);
	}

	return true;
}

TSoftObjectPtr<UWorld> ULRGameContentSet::FindMap(const FName mapId) const
{
	const FLRMapRegistration* found = Maps.FindByPredicate([mapId](const FLRMapRegistration& map)
	{
		return map.MapId == mapId;
	});
	return found ? found->World : TSoftObjectPtr<UWorld>();
}

#if WITH_EDITOR
EDataValidationResult ULRGameContentSet::IsDataValid(FDataValidationContext& context) const
{
	FString error;
	if (!Validate(error))
	{
		context.AddError(FText::FromString(error));
		return EDataValidationResult::Invalid;
	}

	return Super::IsDataValid(context);
}
#endif
