#include "Data/LRGameContentSet.h"

#include "Data/LRLevelEventDefinition.h"
#include "Engine/DataTable.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

namespace
{
	bool ValidateDialogueTable(const UDataTable* dialogueTable, FString& outError)
	{
		const TArray<FName> rowNames = dialogueTable->GetRowNames();
		TSet<FName> dialogueIds;
		for (const FName rowName : rowNames)
		{
			const FLRDialogueRow* row = dialogueTable->FindRow<FLRDialogueRow>(rowName, TEXT("Validate dialogue"));
			if (!row || row->DialogueId.IsNone() || row->DialogueId != rowName || dialogueIds.Contains(row->DialogueId))
			{
				outError = FString::Printf(TEXT("Dialogue row '%s' must have one matching, unique DialogueId."), *rowName.ToString());
				return false;
			}
			dialogueIds.Add(row->DialogueId);
		}

		for (const FName rowName : rowNames)
		{
			const FLRDialogueRow* row = dialogueTable->FindRow<FLRDialogueRow>(rowName, TEXT("Validate dialogue links"));
			if (!row->NextRowId.IsNone() && !dialogueIds.Contains(row->NextRowId))
			{
				outError = FString::Printf(TEXT("Dialogue '%s' references missing NextRowId '%s'."), *rowName.ToString(), *row->NextRowId.ToString());
				return false;
			}

			TSet<FName> optionIds;
			for (const FLRDialogueOption& option : row->Options)
			{
				if (option.OptionId.IsNone() || optionIds.Contains(option.OptionId)
					|| (!option.NextRowId.IsNone() && !dialogueIds.Contains(option.NextRowId)))
				{
					outError = FString::Printf(TEXT("Dialogue '%s' has an empty, duplicate, or unresolved option."), *rowName.ToString());
					return false;
				}
				optionIds.Add(option.OptionId);
			}
		}
		return true;
	}

	bool ValidateReadingTable(const UDataTable* readingTable, FString& outError)
	{
		for (const FName rowName : readingTable->GetRowNames())
		{
			const FLRReadingRow* row = readingTable->FindRow<FLRReadingRow>(rowName, TEXT("Validate reading"));
			if (!row || row->ReadingId.IsNone() || row->ReadingId != rowName)
			{
				outError = FString::Printf(TEXT("Reading row '%s' must have a matching, non-empty ReadingId."), *rowName.ToString());
				return false;
			}
		}
		return true;
	}

	bool ValidateEvents(const TArray<TObjectPtr<ULRLevelEventDefinition>>& events, FString& outError)
	{
		TSet<FName> eventIds;
		for (const ULRLevelEventDefinition* eventDefinition : events)
		{
			if (!eventDefinition || eventDefinition->EventId.IsNone() || eventIds.Contains(eventDefinition->EventId))
			{
				outError = TEXT("LevelEvents contains a missing definition, empty EventId, or duplicate EventId.");
				return false;
			}
			eventIds.Add(eventDefinition->EventId);
		}
		return true;
	}
}

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
	if (!ValidateDialogueTable(DialogueTable, outError) || !ValidateReadingTable(ReadingTable, outError)
		|| !ValidateEvents(LevelEvents, outError))
	{
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

FName ULRGameContentSet::FindMapIdForWorld(const UWorld* world) const
{
	const UPackage* package = world ? world->GetOutermost() : nullptr;
	if (!package)
	{
		return NAME_None;
	}
	const FName packageName = package->GetFName();
	for (const FLRMapRegistration& map : Maps)
	{
		if (map.World.ToSoftObjectPath().GetLongPackageFName() == packageName)
		{
			return map.MapId;
		}
	}
	return NAME_None;
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
