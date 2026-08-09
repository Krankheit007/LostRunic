/**
 * @file LRMovementTuning.h
 * @brief 定义 LostRunic 的内容数据和调优 DataAsset。设计文档中的速度、距离、角度、持续时间、冷却及表现强度都由这里提供编辑器权威值，C++ 默认值仅作安全回退。
 *
 * 关联文件：LRMovementTuning.cpp；所属领域：Data。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Data/LRTuningAsset.h"

#include "LRMovementTuning.generated.h"

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Movement Tuning"))
class LOSTRUNIC_API ULRMovementTuning : public ULRTuningAsset
{
	GENERATED_BODY()

public:
	/** 玩家潜行速度；默认 80 cm/s。 C++ 安全默认值为 `80.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `cm/s`，最小值 `1.0`，最大值 `1000.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Speed", meta = (ClampMin = "1.0", ClampMax = "1000.0", Units = "cm/s"))
	float SneakSpeed = 80.0f;

	/** 玩家走路速度；默认 150 cm/s。 C++ 安全默认值为 `150.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `cm/s`，最小值 `1.0`，最大值 `1000.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Speed", meta = (ClampMin = "1.0", ClampMax = "1000.0", Units = "cm/s"))
	float WalkSpeed = 150.0f;

	/** 玩家奔跑速度；默认 250 cm/s。 C++ 安全默认值为 `250.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `cm/s`，最小值 `1.0`，最大值 `1000.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Speed", meta = (ClampMin = "1.0", ClampMax = "1000.0", Units = "cm/s"))
	float RunSpeed = 250.0f;

	/** Walk Step Distance 的空间距离参数，默认使用 Unreal 厘米单位。 C++ 安全默认值为 `90.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `cm`，最小值 `10.0`，最大值 `500.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Footsteps", meta = (ClampMin = "10.0", ClampMax = "500.0", Units = "cm"))
	float WalkStepDistance = 90.0f;

	/** Run Step Distance 的空间距离参数，默认使用 Unreal 厘米单位。 C++ 安全默认值为 `120.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `cm`，最小值 `10.0`，最大值 `500.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Footsteps", meta = (ClampMin = "10.0", ClampMax = "500.0", Units = "cm"))
	float RunStepDistance = 120.0f;

	/** Sample Interval Seconds 的时间参数，单位为秒；由所属调优或资产提供权威值。 C++ 安全默认值为 `0.1f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `s`，最小值 `0.02`，最大值 `1.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Footsteps", meta = (ClampMin = "0.02", ClampMax = "1.0", Units = "s"))
	float SampleIntervalSeconds = 0.1f;

	/** Indoor Walk Noise Radius 的空间距离参数，默认使用 Unreal 厘米单位。 C++ 安全默认值为 `400.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `cm`，最小值 `0.0`，最大值 `5000.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Noise", meta = (ClampMin = "0.0", ClampMax = "5000.0", Units = "cm"))
	float IndoorWalkNoiseRadius = 400.0f;

	/** Indoor Run Noise Radius 的空间距离参数，默认使用 Unreal 厘米单位。 C++ 安全默认值为 `1200.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `cm`，最小值 `0.0`，最大值 `5000.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Noise", meta = (ClampMin = "0.0", ClampMax = "5000.0", Units = "cm"))
	float IndoorRunNoiseRadius = 1200.0f;

	/** Outdoor Sneak Guard Noise Radius 的空间距离参数，默认使用 Unreal 厘米单位。 C++ 安全默认值为 `600.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `cm`，最小值 `0.0`，最大值 `5000.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Noise", meta = (ClampMin = "0.0", ClampMax = "5000.0", Units = "cm"))
	float OutdoorSneakGuardNoiseRadius = 600.0f;

	/** Outdoor Alert Guard Noise Radius 的空间距离参数，默认使用 Unreal 厘米单位。 C++ 安全默认值为 `250.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `cm`，最小值 `0.0`，最大值 `5000.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Noise", meta = (ClampMin = "0.0", ClampMax = "5000.0", Units = "cm"))
	float OutdoorAlertGuardNoiseRadius = 250.0f;

	/** Interaction Noise Radius 的空间距离参数，默认使用 Unreal 厘米单位。 C++ 安全默认值为 `500.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `cm`，最小值 `0.0`，最大值 `5000.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Noise", meta = (ClampMin = "0.0", ClampMax = "5000.0", Units = "cm"))
	float InteractionNoiseRadius = 500.0f;

	/**
	 * @brief 校验当前资产的必填引用、数值边界及跨字段关系，并输出可诊断错误。
	 * @param outError 输出校验失败原因；成功时保持为空。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	virtual bool Validate(FString& outError) const override;
};
