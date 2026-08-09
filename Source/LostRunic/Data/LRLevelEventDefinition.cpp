/**
 * @file LRLevelEventDefinition.cpp
 * @brief 定义 LostRunic 的内容数据和调优 DataAsset。设计文档中的速度、距离、角度、持续时间、冷却及表现强度都由这里提供编辑器权威值，C++ 默认值仅作安全回退。
 *
 * 关联文件：LRLevelEventDefinition.h；所属领域：Data。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Data/LRLevelEventDefinition.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

/**
 * @brief 查询 Primary Asset Id；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FPrimaryAssetId ULRLevelEventDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("LRLevelEvent"), EventId);
}

#if WITH_EDITOR
/**
 * @brief 接入 Unreal Data Validation，将领域校验错误报告给编辑器。
 * @param context 用于本次条件匹配的 `context` 标签或上下文。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
EDataValidationResult ULRLevelEventDefinition::IsDataValid(FDataValidationContext& context) const
{
	if (EventId.IsNone())
	{
		context.AddError(FText::FromString(TEXT("EventId must be a stable, non-empty name.")));
		return EDataValidationResult::Invalid;
	}

	return Super::IsDataValid(context);
}
#endif
