/**
 * @file LRNPCTuning.h
 * @brief 通用 NPC 的公共调优 DataAsset：玩家朝向检测、噪声反应与巡逻参数；逐 NPC 内容配置在 ULRNPCDefinition，巡逻点按实例配置。
 *
 * 关联文件：LRNPCTuning.cpp；所属领域：Data。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Data/LRTuningAsset.h"

#include "LRNPCTuning.generated.h"

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic NPC Tuning"))
class LOSTRUNIC_API ULRNPCTuning : public ULRTuningAsset
{
	GENERATED_BODY()

public:
	/** NPC 在 Idle 状态检测并朝向玩家的半径；默认 300 cm。 C++ 安全默认值为 `300.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `cm`，最小值 `10.0`，最大值 `5000.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NPC|LookAt", meta = (ClampMin = "10.0", ClampMax = "5000.0", Units = "cm"))
	float LookAtPlayerRadiusCm = 300.0f;

	/** Idle 状态玩家朝向检测的低频间隔；默认 0.25 秒，以计时器代替 Tick。 C++ 安全默认值为 `0.25f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `s`，最小值 `0.05`，最大值 `2.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NPC|LookAt", meta = (ClampMin = "0.05", ClampMax = "2.0", Units = "s"))
	float LookAtIntervalSeconds = 0.25f;

	/** NPC 听见噪声后的限时反应时长；结束后回到配置的默认行为。 C++ 安全默认值为 `3.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `s`，最小值 `0.1`，最大值 `30.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NPC|Noise", meta = (ClampMin = "0.1", ClampMax = "30.0", Units = "s"))
	float NoiseReactionDurationSeconds = 3.0f;

	/** NPC 巡逻移动速度；默认 150 cm/s。 C++ 安全默认值为 `150.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `cm/s`，最小值 `1.0`，最大值 `1000.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NPC|Movement", meta = (ClampMin = "1.0", ClampMax = "1000.0", Units = "cm/s"))
	float PatrolSpeedCm = 150.0f;

	/**
	 * @brief 校验当前资产的必填引用、数值边界及跨字段关系，并输出可诊断错误。
	 * @param outError 输出校验失败原因；成功时保持为空。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	virtual bool Validate(FString& outError) const override;
};
