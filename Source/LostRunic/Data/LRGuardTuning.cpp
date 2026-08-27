/**
 * @file LRGuardTuning.cpp
 * @brief 集中配置守卫总视野角、视野/听觉距离、调查/追逐速度、警戒衰减、搜索和捕获参数，是 AI 行为数值的权威来源。
 *
 * 关联文件：LRGuardTuning.h；所属领域：Data。
 * 设计依据：Docs/Technical/08_ArchitectureBoundaries.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Data/LRGuardTuning.h"

#include "Core/LRGuardConstants.h"

#include "Core/LRValidation.h"

/**
 * @brief 校验当前资产的必填引用、数值边界及跨字段关系，并输出可诊断错误。
 * @param outError 输出校验失败原因；成功时保持为空。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool FLRGuardTuningSettings::Validate(FString& outError) const
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

	if (!LRValidation::RequireRange(TEXT("AttractAlertAmount"), AttractAlertAmount, 1, 11, outError)
		|| !LRValidation::RequireRange(TEXT("DetectionSuspiciousAlertFloor"), DetectionSuspiciousAlertFloor, 1, 11, outError)
		|| !LRValidation::RequireRange(TEXT("DetectionInvestigateAlertFloor"), DetectionInvestigateAlertFloor, 1, 11, outError)
		|| !LRValidation::RequireRange(TEXT("DetectionConfirmedAlertFloor"), DetectionConfirmedAlertFloor, 1, 11, outError)
		|| !LRValidation::RequireRange(TEXT("AlertIncreaseCooldownSeconds"), AlertIncreaseCooldownSeconds, 0.0f, 10.0f, outError)
		|| !LRValidation::RequireRange(TEXT("InvestigateIncreaseCooldownSeconds"), InvestigateIncreaseCooldownSeconds, 0.0f, 10.0f, outError)
		|| !LRValidation::RequireRange(TEXT("RoomRunAlertLevel"), RoomRunAlertLevel, 0, 11, outError)
		|| !LRValidation::RequireRange(TEXT("AdjacentRoomRunAlertAmount"), AdjacentRoomRunAlertAmount, 1, 11, outError)
		|| !LRValidation::RequireRange(TEXT("AlertDecayAmount"), AlertDecayAmount, 1, 11, outError)
		|| !LRValidation::RequireRange(TEXT("InitialObserveSeconds"), InitialObserveSeconds, 0.1f, 30.0f, outError)
		|| !LRValidation::RequireRange(TEXT("AlertDecayIntervalSeconds"), AlertDecayIntervalSeconds, 0.05f, 10.0f, outError)
		|| !LRValidation::RequireRange(TEXT("CaptureRadius"), CaptureRadius, 10.0f, 500.0f, outError)
		|| !LRValidation::RequireRange(TEXT("MoveAcceptanceRadius"), MoveAcceptanceRadius, 1.0f, 500.0f, outError)
		|| !LRValidation::RequireRange(TEXT("InvestigationRetargetDistance"), InvestigationRetargetDistance, 1.0f, 1000.0f, outError)
		|| !LRValidation::RequireRange(TEXT("DetectionSampleIntervalSeconds"), DetectionSampleIntervalSeconds, 0.01f, 1.0f, outError)
		|| !LRValidation::RequireRange(TEXT("MaxDetectionIntegrationDeltaSeconds"), MaxDetectionIntegrationDeltaSeconds, 0.01f, 1.0f, outError)
		|| !LRValidation::RequireRange(TEXT("SuspiciousExposureThresholdSeconds"), SuspiciousExposureThresholdSeconds, 0.0f, 60.0f, outError)
		|| !LRValidation::RequireRange(TEXT("InvestigateExposureThresholdSeconds"), InvestigateExposureThresholdSeconds, 0.0f, 60.0f, outError)
		|| !LRValidation::RequireRange(TEXT("ConfirmedExposureThresholdSeconds"), ConfirmedExposureThresholdSeconds, 0.0f, 60.0f, outError)
		|| !LRValidation::RequireRange(TEXT("DetectionExposureDecayRate"), DetectionExposureDecayRate, 0.0f, 10.0f, outError)
		|| !LRValidation::RequireRange(TEXT("SightEdgeDetectionMultiplier"), SightEdgeDetectionMultiplier, 0.0f, 1.0f, outError)
		|| !LRValidation::RequireRange(TEXT("SneakVisibilityMultiplier"), SneakVisibilityMultiplier, 0.0f, 1.0f, outError)
		|| !LRValidation::RequireRange(TEXT("WalkVisibilityMultiplier"), WalkVisibilityMultiplier, 0.0f, 1.0f, outError)
		|| !LRValidation::RequireRange(TEXT("RunVisibilityMultiplier"), RunVisibilityMultiplier, 0.0f, 1.0f, outError))
	{
		return false;
	}

	if (DetectionSampleIntervalSeconds > MaxDetectionIntegrationDeltaSeconds)
	{
		outError = TEXT("DetectionSampleIntervalSeconds must not exceed MaxDetectionIntegrationDeltaSeconds.");
		return false;
	}

	if (!(DetectionSuspiciousAlertFloor < DetectionInvestigateAlertFloor
		&& DetectionInvestigateAlertFloor < DetectionConfirmedAlertFloor
		&& DetectionConfirmedAlertFloor <= LRGuardConstants::MaxAlertLevel))
	{
		outError = TEXT("Detection alert floors must be strictly increasing and end at or below 11.");
		return false;
	}

	if (SuspiciousExposureThresholdSeconds <= 0.0f
		|| SuspiciousExposureThresholdSeconds >= InvestigateExposureThresholdSeconds
		|| InvestigateExposureThresholdSeconds >= ConfirmedExposureThresholdSeconds)
	{
		outError = TEXT("Detection exposure thresholds must be strictly positive and increasing.");
		return false;
	}

	return true;
}
