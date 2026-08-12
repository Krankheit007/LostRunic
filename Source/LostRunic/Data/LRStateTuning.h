/**
 * @file LRStateTuning.h
 * @brief 定义 LostRunic 的内容数据和调优 DataAsset。设计文档中的速度、距离、角度、持续时间、冷却及表现强度都由这里提供编辑器权威值，C++ 默认值仅作安全回退。
 *
 * 关联文件：LRStateTuning.cpp；所属领域：Data。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Data/LRTuningAsset.h"

#include "LRStateTuning.generated.h"

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic State Tuning"))
class LOSTRUNIC_API ULRStateTuning : public ULRTuningAsset
{
	GENERATED_BODY()

public:
	/** 从 Normal 进入 Perception 或 Courage 的长按阈值；默认 0.8 秒。 C++ 安全默认值为 `0.8f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `s`，最小值 `0.05`，最大值 `5.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "State|Input", meta = (ClampMin = "0.05", ClampMax = "5.0", UIMin = "0.1", UIMax = "2.0", Units = "s"))
	float EnterHoldSeconds = 0.8f;

	/** Exit Hold Seconds 的时间参数，单位为秒；由所属调优或资产提供权威值。 C++ 安全默认值为 `0.3f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `s`，最小值 `0.05`，最大值 `5.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "State|Input", meta = (ClampMin = "0.05", ClampMax = "5.0", UIMin = "0.1", UIMax = "1.0", Units = "s"))
	float ExitHoldSeconds = 0.3f;

	/** Presentation Safety Timeout Seconds 的时间参数，单位为秒；由所属调优或资产提供权威值。 C++ 安全默认值为 `1.5f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `s`，最小值 `0.1`，最大值 `10.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "State|Presentation", meta = (ClampMin = "0.1", ClampMax = "10.0", UIMin = "0.5", UIMax = "3.0", Units = "s"))
	float PresentationSafetyTimeoutSeconds = 1.5f;

	/** Courage Attack Cooldown Seconds 的时间参数，单位为秒；由所属调优或资产提供权威值。 C++ 安全默认值为 `1.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `s`，最小值 `0.0`，最大值 `10.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "State|Courage", meta = (ClampMin = "0.0", ClampMax = "10.0", UIMin = "0.0", UIMax = "3.0", Units = "s"))
	float CourageAttackCooldownSeconds = 1.0f;

	/** Courage Knockback Duration Seconds 的时间参数，单位为秒；由所属调优或资产提供权威值。 C++ 安全默认值为 `0.6f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `s`，最小值 `0.0`，最大值 `5.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "State|Courage", meta = (ClampMin = "0.0", ClampMax = "5.0", UIMin = "0.0", UIMax = "2.0", Units = "s"))
	float CourageKnockbackDurationSeconds = 0.6f;

	/** Courage Knockback Speed 的移动或表现速度，默认使用厘米/秒。 C++ 安全默认值为 `600.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `cm/s`，最小值 `0.0`，最大值 `3000.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "State|Courage", meta = (ClampMin = "0.0", ClampMax = "3000.0", UIMin = "100.0", UIMax = "1200.0", Units = "cm/s"))
	float CourageKnockbackSpeed = 600.0f;

	/** Courage Attack Range 的圆形攻击查询最大距离，默认使用 Unreal 厘米单位。 C++ 安全默认值为 `250.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `cm`，最小值 `1.0`，最大值 `5000.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "State|Courage", meta = (ClampMin = "1.0", ClampMax = "5000.0", UIMin = "100.0", UIMax = "500.0", Units = "cm"))
	float CourageAttackRangeCm = 250.0f;

	/** Courage Attack Facing 的攻击目标完整朝向角；设计基线默认 90 度。 C++ 安全默认值为 `90.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `deg`，最小值 `1.0`，最大值 `360.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "State|Courage", meta = (ClampMin = "1.0", ClampMax = "360.0", UIMin = "30.0", UIMax = "180.0", Units = "deg"))
	float CourageAttackFacingDegrees = 90.0f;

	/**
	 * @brief 校验当前资产的必填引用、数值边界及跨字段关系，并输出可诊断错误。
	 * @param outError 输出校验失败原因；成功时保持为空。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	virtual bool Validate(FString& outError) const override;
};
