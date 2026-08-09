/**
 * @file LRTuningAsset.h
 * @brief 定义 LostRunic 的内容数据和调优 DataAsset。设计文档中的速度、距离、角度、持续时间、冷却及表现强度都由这里提供编辑器权威值，C++ 默认值仅作安全回退。
 *
 * 关联文件：LRTuningAsset.cpp；所属领域：Data。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "LRTuningAsset.generated.h"

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(Abstract, BlueprintType, meta = (DisplayName = "Lost Runic Tuning Asset"))
class LOSTRUNIC_API ULRTuningAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/**
	 * @brief 由各领域调优资产实现必填引用、数值边界和跨字段关系校验。
	 * @param outError 输出首个可诊断错误；校验成功时保持为空。
	 * @return 配置可用于运行时则返回 true，否则返回 false。
	 */
	virtual bool Validate(FString& outError) const PURE_VIRTUAL(ULRTuningAsset::Validate, return false;);

#if WITH_EDITOR
	/**
	 * @brief 接入 Unreal Data Validation，将领域校验错误报告给编辑器。
	 * @param context 用于本次条件匹配的 `context` 标签或上下文。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif
};
