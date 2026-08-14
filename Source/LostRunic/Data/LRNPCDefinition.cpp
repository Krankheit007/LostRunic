/**
 * @file LRNPCDefinition.cpp
 * @brief 通用 NPC 内容定义 DataAsset 的主资产 ID 与编辑器数据校验。
 *
 * 关联文件：LRNPCDefinition.h；所属领域：Data。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Data/LRNPCDefinition.h"

#include "Misc/DataValidation.h"

/**
 * @brief 查询 Primary Asset Id；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FPrimaryAssetId ULRNPCDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("LostRunicNPC"), NpcId);
}

#if WITH_EDITOR
/**
 * @brief 接入 Unreal Data Validation，将领域校验错误报告给编辑器。
 * @param context 用于本次条件匹配的 `context` 标签或上下文。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
EDataValidationResult ULRNPCDefinition::IsDataValid(FDataValidationContext& context) const
{
	if (NpcId.IsNone())
	{
		context.AddError(FText::FromString(TEXT("NpcId must not be empty.")));
	}
	if (!Behavior)
	{
		context.AddError(FText::FromString(TEXT("Behavior StateTree must be assigned.")));
	}
	return context.GetNumErrors() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif
