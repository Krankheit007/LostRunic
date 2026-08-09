/**
 * @file LRGameTuningSet.h
 * @brief 聚合 State、Movement、Interaction、Guard、Save、UI、Presentation 七类调优资产，并统一校验与输出实际来源。
 *
 * 关联文件：LRGameTuningSet.cpp；所属领域：Data。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "LRGameTuningSet.generated.h"

class ULRGuardTuning;
class ULRInteractionTuning;
class ULRMovementTuning;
class ULRPresentationTuning;
class ULRSaveTuning;
class ULRStateTuning;
class ULRUITuning;

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Game Tuning Set"))
class LOSTRUNIC_API ULRGameTuningSet : public UDataAsset
{
	GENERATED_BODY()

public:
	/** State 的领域数据，由所属类型负责维护和校验。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tuning")
	TObjectPtr<ULRStateTuning> State;

	/** Movement 的领域数据，由所属类型负责维护和校验。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tuning")
	TObjectPtr<ULRMovementTuning> Movement;

	/** Interaction Enhanced Input Action 资产；C++ 绑定其语义，具体键位在 Mapping Context 中配置。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tuning")
	TObjectPtr<ULRInteractionTuning> Interaction;

	/** Guard 的领域数据，由所属类型负责维护和校验。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tuning")
	TObjectPtr<ULRGuardTuning> Guard;

	/** Save 的领域数据，由所属类型负责维护和校验。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tuning")
	TObjectPtr<ULRSaveTuning> Save;

	/** UI 的领域数据，由所属类型负责维护和校验。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tuning")
	TObjectPtr<ULRUITuning> UI;

	/** Presentation 的领域数据，由所属类型负责维护和校验。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tuning")
	TObjectPtr<ULRPresentationTuning> Presentation;

	/**
	 * @brief 校验当前资产的必填引用、数值边界及跨字段关系，并输出可诊断错误。
	 * @param outError 输出校验失败原因；成功时保持为空。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool Validate(FString& outError) const;
	/**
	 * @brief 输出聚合调优资产及 State、Movement、Interaction、Guard、Save、UI、Presentation 的实际来源。
	 */
	void LogSources() const;

#if WITH_EDITOR
	/**
	 * @brief 接入 Unreal Data Validation，将领域校验错误报告给编辑器。
	 * @param context 用于本次条件匹配的 `context` 标签或上下文。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif
};
