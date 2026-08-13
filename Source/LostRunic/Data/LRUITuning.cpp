/**
 * @file LRUITuning.cpp
 * @brief 定义 LostRunic 的内容数据和调优 DataAsset。设计文档中的速度、距离、角度、持续时间、冷却及表现强度都由这里提供编辑器权威值，C++ 默认值仅作安全回退。
 *
 * 关联文件：LRUITuning.h；所属领域：Data。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Data/LRUITuning.h"

#include "Core/LRValidation.h"

/**
 * @brief 校验当前资产的必填引用、数值边界及跨字段关系，并输出可诊断错误。
 * @param outError 输出校验失败原因；成功时保持为空。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRUITuning::Validate(FString& outError) const
{
	return LRValidation::RequireRange(TEXT("TypewriterCharactersPerSecond"), TypewriterCharactersPerSecond, 1.0f, 240.0f, outError)
		&& LRValidation::RequireRange(TEXT("TypewriterUpdateSeconds"), TypewriterUpdateSeconds, 0.01f, 0.10f, outError)
		&& LRValidation::RequireRange(TEXT("FailureMessageSeconds"), FailureMessageSeconds, 0.1f, 10.0f, outError)
		&& LRValidation::RequireRange(TEXT("NavigationRepeatSeconds"), NavigationRepeatSeconds, 0.05f, 2.0f, outError)
		&& LRValidation::RequireRange(TEXT("NoteScrollStep"), NoteScrollStep, 1.0f, 500.0f, outError);
}
