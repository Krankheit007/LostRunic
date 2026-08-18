/**
 * @file LRInteractionTuning.h
 * @brief 定义 LostRunic 的内容数据和调优 DataAsset。设计文档中的速度、距离、角度、持续时间、冷却及表现强度都由这里提供编辑器权威值，C++ 默认值仅作安全回退。
 *
 * 关联文件：LRInteractionTuning.cpp；所属领域：Data。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Data/LRTuningAsset.h"

#include "LRInteractionTuning.generated.h"

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Interaction Tuning"))
class LOSTRUNIC_API ULRInteractionTuning : public ULRTuningAsset
{
	GENERATED_BODY()

public:
	/** 远距离粒子提示的最大距离；设计基线默认 500 cm。 C++ 安全默认值为 `1000.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `cm`，最小值 `1.0`，最大值 `5000.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction|Distance", meta = (ClampMin = "1.0", ClampMax = "5000.0", Units = "cm"))
	float FarHintDistance = 2000.0f;

	/** 白色描边提示的最大距离；设计基线默认 200 cm。 C++ 安全默认值为 `500.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `cm`，最小值 `1.0`，最大值 `5000.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction|Distance", meta = (ClampMin = "1.0", ClampMax = "5000.0", Units = "cm"))
	float OutlineDistance = 500.0f;

	/** Execute Distance 的空间距离参数，默认使用 Unreal 厘米单位。 C++ 安全默认值为 `200.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `cm`，最小值 `1.0`，最大值 `5000.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction|Distance", meta = (ClampMin = "1.0", ClampMax = "5000.0", Units = "cm"))
	float ExecuteDistance = 200.0f;

	/** 交互候选的完整朝向范围；设计基线默认总计 90 度。 C++ 安全默认值为 `90.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `deg`，最小值 `1.0`，最大值 `360.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction|Selection", meta = (ClampMin = "1.0", ClampMax = "360.0", Units = "deg"))
	float FacingConeDegrees = 90.0f;

	/** Query Interval Seconds 的时间参数，单位为秒；由所属调优或资产提供权威值。 C++ 安全默认值为 `0.1f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `s`，最小值 `0.02`，最大值 `1.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction|Selection", meta = (ClampMin = "0.02", ClampMax = "1.0", Units = "s"))
	float QueryIntervalSeconds = 0.1f;

	/** Shared vertical lift for world-space interaction prompts; instance overrides live on the presentation component. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction|Presentation", meta = (
		ClampMin = "-1000.0",
		ClampMax = "1000.0",
		Units = "cm"))
	float InteractionPromptZOffset = 40.0f;

	/**
	 * @brief 校验当前资产的必填引用、数值边界及跨字段关系，并输出可诊断错误。
	 * @param outError 输出校验失败原因；成功时保持为空。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	virtual bool Validate(FString& outError) const override;
};
