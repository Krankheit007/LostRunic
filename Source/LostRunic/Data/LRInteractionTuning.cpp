/**
 * @file LRInteractionTuning.cpp
 * @brief 定义 LostRunic 的内容数据和调优 DataAsset。设计文档中的速度、距离、角度、持续时间、冷却及表现强度都由这里提供编辑器权威值，C++ 默认值仅作安全回退。
 *
 * 关联文件：LRInteractionTuning.h；所属领域：Data。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Data/LRInteractionTuning.h"

#include "Core/LRValidation.h"

/**
 * @brief 校验当前资产的必填引用、数值边界及跨字段关系，并输出可诊断错误。
 * @param outError 输出校验失败原因；成功时保持为空。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRInteractionTuning::Validate(FString& outError) const
{
	if (!(ExecuteDistance <= OutlineDistance && OutlineDistance <= FarHintDistance))
	{
		outError = TEXT("Interaction distances must satisfy ExecuteDistance <= OutlineDistance <= FarHintDistance.");
		return false;
	}

	return LRValidation::RequireRange(TEXT("ExecuteDistance"), ExecuteDistance, 1.0f, 5000.0f, outError)
		&& LRValidation::RequireRange(TEXT("OutlineDistance"), OutlineDistance, 1.0f, 5000.0f, outError)
		&& LRValidation::RequireRange(TEXT("FarHintDistance"), FarHintDistance, 1.0f, 5000.0f, outError)
		&& LRValidation::RequireRange(TEXT("FacingConeDegrees"), FacingConeDegrees, 1.0f, 360.0f, outError)
		&& LRValidation::RequireRange(TEXT("QueryIntervalSeconds"), QueryIntervalSeconds, 0.02f, 1.0f, outError);
}
