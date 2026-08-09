/**
 * @file LRProjectSettings.h
 * @brief 定义 LostRunic 的内容数据和调优 DataAsset。设计文档中的速度、距离、角度、持续时间、冷却及表现强度都由这里提供编辑器权威值，C++ 默认值仅作安全回退。
 *
 * 关联文件：Data 目录内调用该公共契约的实现文件；所属领域：Data。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "LRProjectSettings.generated.h"

class ULRGameContentSet;
class ULRGameTuningSet;
class ULRInputConfig;

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Lost Runic"))
class LOSTRUNIC_API ULRProjectSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** Content Set 的领域数据，由所属类型负责维护和校验。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Content")
	TSoftObjectPtr<ULRGameContentSet> ContentSet;

	/** Tuning Set 的领域数据，由所属类型负责维护和校验。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Tuning")
	TSoftObjectPtr<ULRGameTuningSet> TuningSet;

	/** Input Config 的领域数据，由所属类型负责维护和校验。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Input")
	TSoftObjectPtr<ULRInputConfig> InputConfig;

	/**
	 * @brief 查询 Category Name；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	virtual FName GetCategoryName() const override { return TEXT("Game"); }
};
