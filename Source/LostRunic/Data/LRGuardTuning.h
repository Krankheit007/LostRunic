/**
 * @file LRGuardTuning.h
 * @brief 集中配置守卫总视野角、视野/听觉距离、调查/追逐速度、警戒衰减、搜索和捕获参数，是 AI 行为数值的权威来源。
 *
 * 关联文件：LRGuardTuning.cpp；所属领域：Data。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Data/LRTuningAsset.h"

#include "LRGuardTuning.generated.h"

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Guard Tuning"))
class LOSTRUNIC_API ULRGuardTuning : public ULRTuningAsset
{
	GENERATED_BODY()

public:
	/** 守卫确认目标时的视野半径；默认 500 cm（5 m）。 C++ 安全默认值为 `500.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `cm`，最小值 `50.0`，最大值 `5000.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Sight", meta = (ClampMin = "50.0", ClampMax = "5000.0", Units = "cm"))
	float SightRadius = 500.0f;

	/** 目标离开后仍可保持追踪的半径；默认 600 cm（6 m），应不小于 SightRadius。 C++ 安全默认值为 `600.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `cm`，最小值 `50.0`，最大值 `5000.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Sight", meta = (ClampMin = "50.0", ClampMax = "5000.0", Units = "cm"))
	float LoseSightRadius = 600.0f;

	/** 守卫完整视野锥总角度；默认 45 度，传给 UE Sight 时换算为半角。 C++ 安全默认值为 `45.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `deg`，最小值 `1.0`，最大值 `180.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Sight", meta = (ClampMin = "1.0", ClampMax = "180.0", Units = "deg", ToolTip = "Full sight cone; UE perception receives half this value."))
	float SightConeDegrees = 45.0f;

	/** 对噪声事件半径应用的听觉倍率；默认 1。 C++ 安全默认值为 `1.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：最小值 `0.0`，最大值 `10.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Hearing", meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float HearingRangeMultiplier = 1.0f;

	/** 守卫可响应噪声的最大距离；默认 5000 cm。 C++ 安全默认值为 `5000.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `cm`，最小值 `50.0`，最大值 `10000.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Hearing", meta = (ClampMin = "50.0", ClampMax = "10000.0", Units = "cm"))
	float MaxHearingRange = 5000.0f;

	/** 有效听觉刺激首次增加的警戒量；默认 6。 C++ 安全默认值为 `6`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：最小值 `1`，最大值 `11`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Alert", meta = (ClampMin = "1", ClampMax = "11"))
	int32 HearingAlertAmount = 6;

	/** 明确看见玩家后设置的警戒等级；默认 11，进入追逐。 C++ 安全默认值为 `11`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：最小值 `1`，最大值 `11`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Alert", meta = (ClampMin = "1", ClampMax = "11"))
	int32 SightAlertLevel = 11;

	/** 每个衰减周期降低的警戒值；默认 1。 C++ 安全默认值为 `1`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：最小值 `1`，最大值 `11`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Alert", meta = (ClampMin = "1", ClampMax = "11"))
	int32 AlertDecayAmount = 1;

	/** 守卫调查异常位置时的速度；默认 170 cm/s。 C++ 安全默认值为 `170.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `cm/s`，最小值 `1.0`，最大值 `1000.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Movement", meta = (ClampMin = "1.0", ClampMax = "1000.0", Units = "cm/s"))
	float InvestigateSpeed = 170.0f;

	/** 守卫追逐玩家时的速度；默认 300 cm/s。 C++ 安全默认值为 `300.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `cm/s`，最小值 `1.0`，最大值 `1000.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Movement", meta = (ClampMin = "1.0", ClampMax = "1000.0", Units = "cm/s"))
	float ChaseSpeed = 300.0f;

	/** 警戒从 0 提升后的初始观察时长；默认 3 秒。 C++ 安全默认值为 `3.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `s`，最小值 `0.1`，最大值 `30.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Alert", meta = (ClampMin = "0.1", ClampMax = "30.0", Units = "s"))
	float InitialObserveSeconds = 3.0f;

	/** 低警戒自然下降的间隔；默认 0.5 秒。 C++ 安全默认值为 `0.5f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `s`，最小值 `0.05`，最大值 `10.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Alert", meta = (ClampMin = "0.05", ClampMax = "10.0", Units = "s"))
	float AlertDecayIntervalSeconds = 0.5f;

	/** 到达最后异常位置后的搜索持续时间；默认 5 秒。 C++ 安全默认值为 `5.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `s`，最小值 `0.1`，最大值 `60.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Alert", meta = (ClampMin = "0.1", ClampMax = "60.0", Units = "s"))
	float SearchDurationSeconds = 5.0f;

	/** 追逐中判定捕获玩家的距离；默认 75 cm。 C++ 安全默认值为 `75.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `cm`，最小值 `10.0`，最大值 `500.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Capture", meta = (ClampMin = "10.0", ClampMax = "500.0", Units = "cm"))
	float CaptureRadius = 75.0f;

	/** 捕获距离检查周期；默认 0.1 秒，以计时器代替 Tick。 C++ 安全默认值为 `0.1f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `s`，最小值 `0.02`，最大值 `1.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Capture", meta = (ClampMin = "0.02", ClampMax = "1.0", Units = "s"))
	float CaptureCheckIntervalSeconds = 0.1f;

	/** 守卫导航到调查点时允许的到达误差；默认 50 cm。 C++ 安全默认值为 `50.0f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `cm`，最小值 `1.0`，最大值 `500.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Movement", meta = (ClampMin = "1.0", ClampMax = "500.0", Units = "cm"))
	float MoveAcceptanceRadius = 50.0f;

	/**
	 * @brief 校验当前资产的必填引用、数值边界及跨字段关系，并输出可诊断错误。
	 * @param outError 输出校验失败原因；成功时保持为空。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	virtual bool Validate(FString& outError) const override;
};
