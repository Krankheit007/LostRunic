/**
 * @file LRSaveTuning.h
 * @brief 定义 LostRunic 的内容数据和调优 DataAsset。设计文档中的速度、距离、角度、持续时间、冷却及表现强度都由这里提供编辑器权威值，C++ 默认值仅作安全回退。
 *
 * 关联文件：LRSaveTuning.cpp；所属领域：Data。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Data/LRTuningAsset.h"

#include "LRSaveTuning.generated.h"

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Save Tuning"))
class LOSTRUNIC_API ULRSaveTuning : public ULRTuningAsset
{
	GENERATED_BODY()

public:
	/** 普通自动存档防抖时长；默认 7.5 秒，关键 Memory 写入不使用该防抖。 C++ 安全默认值为 `7.5f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `s`，最小值 `0.0`，最大值 `60.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Save|Autosave", meta = (ClampMin = "0.0", ClampMax = "60.0", Units = "s"))
	float AutoSaveDebounceSeconds = 7.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Save|Autosave")
	bool bAutoSaveAfterMapReady = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Save|Autosave")
	bool bAutoSaveImportantStoryEvents = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Save|Autosave")
	bool bAutoSaveSuccessfulInteractions = false;

	/** Retry Count 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `2`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：最小值 `0`，最大值 `10`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Save|Retry", meta = (ClampMin = "0", ClampMax = "10"))
	int32 RetryCount = 2;

	/** Retry Delay Seconds 的时间参数，单位为秒；由所属调优或资产提供权威值。 C++ 安全默认值为 `0.5f`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：单位 `s`，最小值 `0.0`，最大值 `10.0`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Save|Retry", meta = (ClampMin = "0.0", ClampMax = "10.0", Units = "s"))
	float RetryDelaySeconds = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Save|Reliability", meta = (ClampMin = "1.0", ClampMax = "120.0", Units = "s"))
	float OperationTimeoutSeconds = 30.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Save|Reliability", meta = (ClampMin = "1.0", ClampMax = "120.0", Units = "s"))
	float AsyncWatchdogSeconds = 15.0f;

	/** Manual Slot Count 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `10`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。编辑器约束：最小值 `1`，最大值 `100`。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Save|Slots", meta = (ClampMin = "1", ClampMax = "100"))
	int32 MaxManualSaveSlots = 20;

	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use MaxManualSaveSlots."))
	int32 ManualSlotCount = 10;

	/**
	 * @brief 校验当前资产的必填引用、数值边界及跨字段关系，并输出可诊断错误。
	 * @param outError 输出校验失败原因；成功时保持为空。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	virtual bool Validate(FString& outError) const override;
};
