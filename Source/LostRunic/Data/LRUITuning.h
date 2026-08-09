/**
 * @file LRUITuning.h
 * @brief 定义 LostRunic 的内容数据和调优 DataAsset。设计文档中的速度、距离、角度、持续时间、冷却及表现强度都由这里提供编辑器权威值，C++ 默认值仅作安全回退。
 *
 * 关联文件：LRUITuning.cpp；所属领域：Data。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Data/LRTuningAsset.h"

#include "LRUITuning.generated.h"

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic UI Tuning"))
class LOSTRUNIC_API ULRUITuning : public ULRTuningAsset
{
	GENERATED_BODY()

public:
	/** 对话打字机每秒显示字符数。 C++ 安全默认值为 `30.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：最小值 `1.0`，最大值 `240.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Dialogue", meta = (ClampMin = "1.0", ClampMax = "240.0", UIMin = "5.0", UIMax = "120.0", ToolTip = "Number of visible dialogue characters revealed per second."))
	float TypewriterCharactersPerSecond = 30.0f;

	/** Typewriter Update Seconds 的时间参数，单位为秒；由所属调优或资产提供权威值。 C++ 安全默认值为 `0.033f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `s`，最小值 `0.01`，最大值 `0.10`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Dialogue", meta = (ClampMin = "0.01", ClampMax = "0.10", UIMin = "0.02", UIMax = "0.05", Units = "s", ToolTip = "How often the timer refreshes displayed typewriter text."))
	float TypewriterUpdateSeconds = 0.033f;

	/** Failure Message Seconds 的时间参数，单位为秒；由所属调优或资产提供权威值。 C++ 安全默认值为 `2.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `s`，最小值 `0.1`，最大值 `10.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Feedback", meta = (ClampMin = "0.1", ClampMax = "10.0", Units = "s"))
	float FailureMessageSeconds = 2.0f;

	/** Navigation Repeat Seconds 的时间参数，单位为秒；由所属调优或资产提供权威值。 C++ 安全默认值为 `0.2f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `s`，最小值 `0.05`，最大值 `2.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Input", meta = (ClampMin = "0.05", ClampMax = "2.0", Units = "s"))
	float NavigationRepeatSeconds = 0.2f;

	/**
	 * @brief 校验当前资产的必填引用、数值边界及跨字段关系，并输出可诊断错误。
	 * @param outError 输出校验失败原因；成功时保持为空。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	virtual bool Validate(FString& outError) const override;
};
