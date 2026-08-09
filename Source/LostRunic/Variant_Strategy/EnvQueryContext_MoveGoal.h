// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @file EnvQueryContext_MoveGoal.h
 * @brief 保留 Unreal Strategy 模板玩法，用于回归和 PIE 冒烟；它与 /Game/LostRunic 的“家”切片相互独立，不承载 LostRunic 核心叙事规则。
 *
 * 关联文件：EnvQueryContext_MoveGoal.cpp；所属领域：Variant_Strategy。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "EnvQueryContext_MoveGoal.generated.h"

/**
 *  Simple EnvQueryContext that returns a Unit's current movement goal location
 */
UCLASS()
class LOSTRUNIC_API UEnvQueryContext_MoveGoal : public UEnvQueryContext
{
	GENERATED_BODY()

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Provide Context 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 * @param QueryInstance 调用方提供的 `QueryInstance`，只在本次操作范围内使用。
	 * @param ContextData 调用方提供的 `ContextData`，只在本次操作范围内使用。
	 */
	virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;
};
