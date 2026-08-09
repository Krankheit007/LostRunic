/**
 * @file LRPresentationTuning.h
 * @brief 定义 LostRunic 的内容数据和调优 DataAsset。设计文档中的速度、距离、角度、持续时间、冷却及表现强度都由这里提供编辑器权威值，C++ 默认值仅作安全回退。
 *
 * 关联文件：LRPresentationTuning.cpp；所属领域：Data。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Data/LRTuningAsset.h"

#include "LRPresentationTuning.generated.h"

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Presentation Tuning"))
class LOSTRUNIC_API ULRPresentationTuning : public ULRTuningAsset
{
	GENERATED_BODY()

public:
	/** Perception Reveal Radius 的空间距离参数，默认使用 Unreal 厘米单位。 C++ 安全默认值为 `450.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `cm`，最小值 `0.0`，最大值 `5000.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Perception", meta = (ClampMin = "0.0", ClampMax = "5000.0", Units = "cm"))
	float PerceptionRevealRadius = 450.0f;

	/** Noise Reveal Radius 的空间距离参数，默认使用 Unreal 厘米单位。 C++ 安全默认值为 `200.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `cm`，最小值 `0.0`，最大值 `5000.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Perception", meta = (ClampMin = "0.0", ClampMax = "5000.0", Units = "cm"))
	float NoiseRevealRadius = 200.0f;

	/** Noise Reveal Duration Seconds 的时间参数，单位为秒；由所属调优或资产提供权威值。 C++ 安全默认值为 `5.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `s`，最小值 `0.0`，最大值 `30.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Perception", meta = (ClampMin = "0.0", ClampMax = "30.0", Units = "s"))
	float NoiseRevealDurationSeconds = 5.0f;

	/** Perception Blend Weight 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `1.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：最小值 `0.0`，最大值 `1.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|PostProcess", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PerceptionBlendWeight = 1.0f;

	/** Courage Blend Weight 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `1.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：最小值 `0.0`，最大值 `1.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|PostProcess", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CourageBlendWeight = 1.0f;

	/**
	 * @brief 校验当前资产的必填引用、数值边界及跨字段关系，并输出可诊断错误。
	 * @param outError 输出校验失败原因；成功时保持为空。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	virtual bool Validate(FString& outError) const override;
};
