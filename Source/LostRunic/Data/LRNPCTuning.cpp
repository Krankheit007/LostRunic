/**
 * @file LRNPCTuning.cpp
 * @brief 通用 NPC 公共调优 DataAsset 的校验实现。
 *
 * 关联文件：LRNPCTuning.h；所属领域：Data。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Data/LRNPCTuning.h"

#include "Core/LRValidation.h"

/**
 * @brief 校验当前资产的必填引用、数值边界及跨字段关系，并输出可诊断错误。
 * @param outError 输出校验失败原因；成功时保持为空。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRNPCTuning::Validate(FString& outError) const
{
	return LRValidation::RequireRange(TEXT("LookAtPlayerRadiusCm"), LookAtPlayerRadiusCm, 10.0f, 5000.0f, outError)
		&& LRValidation::RequireRange(TEXT("LookAtIntervalSeconds"), LookAtIntervalSeconds, 0.05f, 2.0f, outError)
		&& LRValidation::RequireRange(TEXT("NoiseReactionDurationSeconds"), NoiseReactionDurationSeconds, 0.1f, 30.0f, outError)
		&& LRValidation::RequireRange(TEXT("PatrolSpeedCm"), PatrolSpeedCm, 1.0f, 1000.0f, outError);
}
