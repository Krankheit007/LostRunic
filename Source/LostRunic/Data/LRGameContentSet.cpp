/**
 * @file LRGameContentSet.cpp
 * @brief 聚合 Dialogue/Reading 表、物品/收藏品/守卫/关卡事件定义和地图软引用注册，避免运行时代码散落硬编码 /Game 路径。
 *
 * 关联文件：LRGameContentSet.h；所属领域：Data。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Data/LRGameContentSet.h"

#include "Data/LRCollectibleDefinition.h"
#include "Data/LRGuardDefinition.h"
#include "Data/LRItemDefinition.h"
#include "Data/LRLevelEventDefinition.h"
#include "Engine/DataTable.h"
#include "Internationalization/StringTable.h"
#include "Items/LRInventoryComponent.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

namespace
{
	/**
	 * @brief 执行 Validate Dialogue Table 的纯规则或事务判定，失败时提供结构化原因。
	 * @param dialogueTable 数据或调优来源 `dialogueTable`；调用期间只读，并按稳定 ID 解析内容。
	 * @param outError 输出校验失败原因；成功时保持为空。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
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

	/**
	 * @brief 执行 Validate Reading Table 的纯规则或事务判定，失败时提供结构化原因。
	 * @param readingTable 数据或调优来源 `readingTable`；调用期间只读，并按稳定 ID 解析内容。
	 * @param outError 输出校验失败原因；成功时保持为空。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool ValidateReadingTable(const UDataTable* readingTable, FString& outError)
	{
		const TArray<FName> rowNames = readingTable->GetRowNames();
		if (!ULRGameContentSet::IsReadingCapacityWithinLimits(rowNames.Num()))
		{
			outError = FString::Printf(TEXT("ReadingTable has %d rows; menu supports at most %d notes."),
				rowNames.Num(), LRMenuCapacity::Notes);
			return false;
		}
		for (const FName rowName : rowNames)
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

	/**
	 * @brief 校验收藏品定义数量不超过菜单容量；共享命名限制为 12 件。
	 * @param collectibles 数据或调优来源 `collectibles`；调用期间只读，并按稳定 ID 解析内容。
	 * @param outError 输出校验失败原因；成功时保持为空。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool ValidateCollectibleCapacity(const TArray<TObjectPtr<ULRCollectibleDefinition>>& collectibles, FString& outError)
	{
		if (!ULRGameContentSet::IsCollectibleCapacityWithinLimits(collectibles.Num()))
		{
			outError = FString::Printf(TEXT("Collectibles has %d definitions; menu supports at most %d."),
				collectibles.Num(), LRMenuCapacity::Collectibles);
			return false;
		}
		return true;
	}

	/**
	 * @brief 执行 Validate Events 的纯规则或事务判定，失败时提供结构化原因。
	 * @param events 本次领域操作的结构化数据 `events`；字段语义由对应 USTRUCT 定义。
	 * @param outError 输出校验失败原因；成功时保持为空。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
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

	template <typename DefinitionType, typename GetIdFunc>
	bool ValidateDefinitionIds(const TArray<TObjectPtr<DefinitionType>>& definitions,
		GetIdFunc getId, const TCHAR* label, FString& outError)
	{
		TSet<FName> ids;
		for (const DefinitionType* definition : definitions)
		{
			const FName id = definition ? getId(*definition) : NAME_None;
			if (!definition || id.IsNone() || ids.Contains(id))
			{
				outError = FString::Printf(TEXT("%s contains a missing definition, empty ID, or duplicate ID."), label);
				return false;
			}
			ids.Add(id);
		}
		return true;
	}
}

/**
 * @brief 校验当前资产的必填引用、数值边界及跨字段关系，并输出可诊断错误。
 * @param outError 输出校验失败原因；成功时保持为空。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRGameContentSet::Validate(FString& outError) const
{
	if (!UIStringTable)
	{
		outError = TEXT("UIStringTable must be assigned for localized content.");
		return false;
	}
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
		|| !ValidateEvents(LevelEvents, outError)
		|| !ValidateDefinitionIds(Items, [](const ULRItemDefinition& definition) -> FName { return definition.ItemId; }, TEXT("Items"), outError)
		|| !ValidateDefinitionIds(Collectibles,
			[](const ULRCollectibleDefinition& definition) -> FName { return definition.CollectibleId; }, TEXT("Collectibles"), outError)
		|| !ValidateCollectibleCapacity(Collectibles, outError)
		|| !ValidateDefinitionIds(Guards, [](const ULRGuardDefinition& definition) -> FName { return definition.GuardId; }, TEXT("Guards"), outError))
	{
		return false;
	}

	TSet<FName> mapIds;
	for (const FLRMapRegistration& map : Maps)
	{
		if (map.MapId.IsNone() || map.DisplayNameTextKey.IsNone() || map.World.IsNull() || mapIds.Contains(map.MapId)
			|| (map.bPlayableMap && map.DefaultStartAnchorId.IsNone()))
		{
			outError = FString::Printf(TEXT("Map registration '%s' is missing a world, localized DisplayNameTextKey, or valid identity."), *map.MapId.ToString());
			return false;
		}
		mapIds.Add(map.MapId);
	}
	if (NewGameMapId.IsNone() || !mapIds.Contains(NewGameMapId))
	{
		outError = TEXT("NewGameMapId must resolve to a registered map.");
		return false;
	}
	if (MainMenuMapId.IsNone() || !mapIds.Contains(MainMenuMapId))
	{
		outError = TEXT("MainMenuMapId must resolve to a registered map.");
		return false;
	}

	return true;
}

/**
 * @brief 共享命名容量契约：ReadingTable 行数（笔记）不超过 12；供编辑器校验与测试直接断言规则。
 * @param readingRowCount 本次操作使用的计数、增量或索引 `readingRowCount`；由函数校验合法范围。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRGameContentSet::IsReadingCapacityWithinLimits(const int32 readingRowCount)
{
	return readingRowCount <= LRMenuCapacity::Notes;
}

/**
 * @brief 共享命名容量契约：收藏品定义数不超过 12；供编辑器校验与测试直接断言规则。
 * @param collectibleCount 本次操作使用的计数、增量或索引 `collectibleCount`；由函数校验合法范围。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRGameContentSet::IsCollectibleCapacityWithinLimits(const int32 collectibleCount)
{
	return collectibleCount <= LRMenuCapacity::Collectibles;
}

/**
 * @brief 按稳定 ID 或运行时条件查找 Map，未找到时返回明确失败值。
 * @param mapId 稳定标识 `mapId`；用于内容查询和存档，不依赖显示名或数组序号。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
TSoftObjectPtr<UWorld> ULRGameContentSet::FindMap(const FName mapId) const
{
	const FLRMapRegistration* found = Maps.FindByPredicate([mapId](const FLRMapRegistration& map)
	{
		return map.MapId == mapId;
	});
	return found ? found->World : TSoftObjectPtr<UWorld>();
}

const FLRMapRegistration* ULRGameContentSet::FindMapRegistration(const FName mapId) const
{
	return Maps.FindByPredicate([mapId](const FLRMapRegistration& map) { return map.MapId == mapId; });
}

FText ULRGameContentSet::ResolveUIText(const FName textKey) const
{
	if (UIStringTable && !textKey.IsNone())
	{
		const FText localizedText = FText::FromStringTable(UIStringTable->GetStringTableId(), textKey.ToString());
		if (!localizedText.IsEmpty())
		{
			return localizedText;
		}
	}
	return FText::FromName(textKey);
}

FText ULRGameContentSet::GetMapDisplayName(const FName mapId) const
{
	const FLRMapRegistration* map = FindMapRegistration(mapId);
	if (!map)
	{
		return FText::FromName(mapId);
	}
	if (!map->DisplayNameTextKey.IsNone() && UIStringTable)
	{
		return ResolveUIText(map->DisplayNameTextKey);
	}
	return FText::FromName(mapId);
}

ULRItemDefinition* ULRGameContentSet::FindItemDefinition(const FName itemId) const
{
	const TObjectPtr<ULRItemDefinition>* found = Items.FindByPredicate([itemId](const TObjectPtr<ULRItemDefinition>& definition)
	{
		return definition && definition->ItemId == itemId;
	});
	return found ? found->Get() : nullptr;
}

ULRCollectibleDefinition* ULRGameContentSet::FindCollectibleDefinition(const FName collectibleId) const
{
	const TObjectPtr<ULRCollectibleDefinition>* found = Collectibles.FindByPredicate(
		[collectibleId](const TObjectPtr<ULRCollectibleDefinition>& definition)
		{
			return definition && definition->CollectibleId == collectibleId;
		});
	return found ? found->Get() : nullptr;
}

ULRGuardDefinition* ULRGameContentSet::FindGuardDefinition(const FName guardId) const
{
	const TObjectPtr<ULRGuardDefinition>* found = Guards.FindByPredicate([guardId](const TObjectPtr<ULRGuardDefinition>& definition)
	{
		return definition && definition->GuardId == guardId;
	});
	return found ? found->Get() : nullptr;
}

/**
 * @brief 按稳定 ID 或运行时条件查找 Map Id For World，未找到时返回明确失败值。
 * @param world 要解析地图 ID、应用恢复状态或执行查询的 Unreal World。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
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
/**
 * @brief 接入 Unreal Data Validation，将领域校验错误报告给编辑器。
 * @param context 用于本次条件匹配的 `context` 标签或上下文。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
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
