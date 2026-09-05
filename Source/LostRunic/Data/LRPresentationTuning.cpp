/**
 * @file LRPresentationTuning.cpp
 * @brief 定义 LostRunic 的内容数据和调优 DataAsset。设计文档中的速度、距离、角度、持续时间、冷却及表现强度都由这里提供编辑器权威值，C++ 默认值仅作安全回退。
 *
 * 关联文件：LRPresentationTuning.h；所属领域：Data。
 * 设计依据：Docs/Technical/08_ArchitectureBoundaries.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Data/LRPresentationTuning.h"

#include "Core/LRValidation.h"

// Keep validation in this translation unit so the generated UObject layout is rebuilt with the tuning contract.

/**
 * @brief 校验当前资产的必填引用、数值边界及跨字段关系，并输出可诊断错误。
 * @param outError 输出校验失败原因；成功时保持为空。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRPresentationTuning::Validate(FString& outError) const
{
	if (PerceptionCompositeMaterial.IsNull())
	{
		outError = TEXT("PerceptionCompositeMaterial must be assigned.");
		return false;
	}
	if (!PerceptionVisualStyleParameterCollection)
	{
		outError = TEXT("PerceptionVisualStyleParameterCollection must be assigned.");
		return false;
	}
	if (!PerceptionRuntimeParameterCollection)
	{
		outError = TEXT("PerceptionRuntimeParameterCollection must be assigned.");
		return false;
	}

	return LRValidation::RequireRange(TEXT("PerceptionRevealRadius"), PerceptionRevealRadius, 0.0f, 5000.0f, outError)
		&& LRValidation::RequireRange(TEXT("NoiseRevealRadius"), NoiseRevealRadius, 0.0f, 5000.0f, outError)
		&& LRValidation::RequireRange(TEXT("NoiseRevealDurationSeconds"), NoiseRevealDurationSeconds, 0.0f, 30.0f, outError)
		&& LRValidation::RequireRange(TEXT("PerceptionFullRevealRadius"), PerceptionFullRevealRadius, 0.0f, 5000.0f, outError)
		&& LRValidation::RequireRange(TEXT("EchoExpansionSeconds"), EchoExpansionSeconds, 0.001f, 30.0f, outError)
		&& LRValidation::RequireRange(TEXT("EchoWetSeconds"), EchoWetSeconds, 0.0f, 30.0f, outError)
		&& LRValidation::RequireRange(TEXT("EchoDryFadeDurationSeconds"), EchoDryFadeDurationSeconds, 0.001f, 30.0f, outError)
		&& LRValidation::RequireRange(TEXT("EchoWaveWidthCm"), EchoWaveWidthCm, 0.0f, 1000.0f, outError)
		&& LRValidation::RequireRange(TEXT("EchoRefreshMergeDistanceCm"), EchoRefreshMergeDistanceCm, 0.0f, 1000.0f, outError)
		&& LRValidation::RequireRange(TEXT("PerceptionBoundaryNoiseCm"), PerceptionBoundaryNoiseCm, 0.0f, 1000.0f, outError)
		&& LRValidation::RequireRange(TEXT("DefaultLoopIntervalSeconds"), DefaultLoopIntervalSeconds, 0.001f, 60.0f, outError)
		&& LRValidation::RequireRange(TEXT("AccentDepthToleranceCm"), AccentDepthToleranceCm, 0.0f, 100.0f, outError)
		&& LRValidation::RequireRange(TEXT("PerceptionEnterBlendSeconds"), PerceptionEnterBlendSeconds, 0.0f, 5.0f, outError)
		&& LRValidation::RequireRange(TEXT("PerceptionExitBlendSeconds"), PerceptionExitBlendSeconds, 0.0f, 5.0f, outError)
		&& LRValidation::RequireRange(TEXT("PerceptionBlendWeight"), PerceptionBlendWeight, 0.0f, 1.0f, outError)
		&& LRValidation::RequireRange(TEXT("CourageBlendWeight"), CourageBlendWeight, 0.0f, 1.0f, outError)
		&& LRValidation::RequireRange(TEXT("EyeOverlaySuccessTailSeconds"), EyeOverlaySuccessTailSeconds, 0.0f, 1.0f, outError)
		&& LRValidation::RequireRange(TEXT("EyeOverlayCancelSeconds"), EyeOverlayCancelSeconds, 0.0f, 1.0f, outError)
		&& LRValidation::RequireRange(TEXT("EyeOpenThresholdVisualProgress"), EyeOpenThresholdVisualProgress, 0.0f, 1.0f, outError)
		&& LRValidation::RequireRange(TEXT("CutawayHideDurationSeconds"), CutawayHideDurationSeconds, 0.0f, 2.0f, outError)
		&& LRValidation::RequireRange(TEXT("CutawayRestoreDurationSeconds"), CutawayRestoreDurationSeconds, 0.0f, 2.0f, outError)
		&& LRValidation::RequireRange(TEXT("CutawayRadiusRefPx"), CutawayRadiusRefPx, 32.0f, 600.0f, outError)
		&& LRValidation::RequireRange(TEXT("CutawayDetectionFrequencyHz"), CutawayDetectionFrequencyHz, 1.0f, 60.0f, outError)
		&& LRValidation::RequireRange(TEXT("CutawayTraceSphereRadiusCm"), CutawayTraceSphereRadiusCm, 0.0f, 100.0f, outError)
		&& LRValidation::RequireRange(TEXT("DefaultCameraDistanceCm"), DefaultCameraDistanceCm, 300.0f, 1400.0f, outError)
		&& LRValidation::RequireRange(TEXT("DefaultCameraDistanceBlendSeconds"), DefaultCameraDistanceBlendSeconds, 0.0f, 5.0f, outError);
}
