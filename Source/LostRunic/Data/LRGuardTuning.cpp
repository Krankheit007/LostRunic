/**
 * @file LRGuardTuning.cpp
 * @brief 集中配置守卫总视野角、视野/听觉距离、调查/追逐速度、警戒衰减、搜索和捕获参数，是 AI 行为数值的权威来源。
 *
 * 关联文件：LRGuardTuning.h；所属领域：Data。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Data/LRGuardTuning.h"

#include "Core/LRValidation.h"

/**
 * @brief 校验当前资产的必填引用、数值边界及跨字段关系，并输出可诊断错误。
 * @param outError 输出校验失败原因；成功时保持为空。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRGuardTuning::Validate(FString& outError) const
{
	if (!LRValidation::RequireRange(TEXT("InvestigateSpeed"), InvestigateSpeed, 1.0f, 1000.0f, outError)
		|| !LRValidation::RequireRange(TEXT("ChaseSpeed"), ChaseSpeed, 1.0f, 1000.0f, outError))
	{
		return false;
	}

	if (InvestigateSpeed > ChaseSpeed)
	{
		outError = TEXT("InvestigateSpeed must not exceed ChaseSpeed.");
		return false;
	}

	return LRValidation::RequireRange(TEXT("SightRadius"), SightRadius, 50.0f, 5000.0f, outError)
		&& LRValidation::RequireRange(TEXT("LoseSightRadius"), LoseSightRadius, 50.0f, 5000.0f, outError)
		&& LRValidation::RequireRange(TEXT("SightConeDegrees"), SightConeDegrees, 1.0f, 180.0f, outError)
		&& LRValidation::RequireRange(TEXT("HearingRangeMultiplier"), HearingRangeMultiplier, 0.0f, 10.0f, outError)
		&& LRValidation::RequireRange(TEXT("MaxHearingRange"), MaxHearingRange, 50.0f, 10000.0f, outError)
		&& LRValidation::RequireRange(TEXT("HearingAlertAmount"), HearingAlertAmount, 1, 11, outError)
		&& LRValidation::RequireRange(TEXT("SightAlertLevel"), SightAlertLevel, 1, 11, outError)
		&& LRValidation::RequireRange(TEXT("AlertDecayAmount"), AlertDecayAmount, 1, 11, outError)
		&& LRValidation::RequireRange(TEXT("InitialObserveSeconds"), InitialObserveSeconds, 0.1f, 30.0f, outError)
		&& LRValidation::RequireRange(TEXT("AlertDecayIntervalSeconds"), AlertDecayIntervalSeconds, 0.05f, 10.0f, outError)
		&& LRValidation::RequireRange(TEXT("SearchDurationSeconds"), SearchDurationSeconds, 0.1f, 60.0f, outError)
		&& LRValidation::RequireRange(TEXT("CaptureRadius"), CaptureRadius, 10.0f, 500.0f, outError)
		&& LRValidation::RequireRange(TEXT("CaptureCheckIntervalSeconds"), CaptureCheckIntervalSeconds, 0.02f, 1.0f, outError)
		&& LRValidation::RequireRange(TEXT("MoveAcceptanceRadius"), MoveAcceptanceRadius, 1.0f, 500.0f, outError)
		&& LoseSightRadius >= SightRadius;
}
