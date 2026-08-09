// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @file TwinStickStateTreeUtility.h
 * @brief 保留 Unreal TwinStick 模板玩法，用于回归和 PIE 冒烟；它与 /Game/LostRunic 的“家”切片相互独立，不承载 LostRunic 核心叙事规则。
 *
 * 关联文件：TwinStickStateTreeUtility.cpp；所属领域：Variant_TwinStick。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"

#include "TwinStickStateTreeUtility.generated.h"

class ACharacter;

/**
 *  Instance data struct for the Get Player task
 */
USTRUCT()
struct FStateTreeGetPlayerInstanceData
{
	GENERATED_BODY()

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Character 的领域数据，由所属类型负责维护和校验。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, Category="Context")
	TObjectPtr<ACharacter> Character;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Target Player Character 的领域数据，由所属类型负责维护和校验。 仅在蓝图或详情面板中查看，不可编辑。 */
	UPROPERTY(VisibleAnywhere, Category="Output")
	TObjectPtr<ACharacter> TargetPlayerCharacter;
};

/**
 *  StateTree task to get the player character
 */
USTRUCT(meta=(DisplayName="GetPlayer", Category="TwinStick"))
struct FStateTreeGetPlayerTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	/* Ensure we're using the correct instance data struct */
	using FInstanceDataType = FStateTreeGetPlayerInstanceData;
	/**
	 * @brief 查询 Instance Data Type；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Tick 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 * @param Context 用于本次条件匹配的 `Context` 标签或上下文。
	 * @param DeltaTime 时间值 `DeltaTime`，单位为秒。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

#if WITH_EDITOR
	/**
	 * @brief 查询 Description；不修改领域状态。
	 * @param ID 稳定标识 `ID`；用于内容查询和存档，不依赖显示名或数组序号。
	 * @param InstanceDataView 调用方提供的 `InstanceDataView`，只在本次操作范围内使用。
	 * @param BindingLookup 调用方提供的 `BindingLookup`，只在本次操作范围内使用。
	 * @param Formatting 调用方提供的 `Formatting`，只在本次操作范围内使用。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif // WITH_EDITOR
};
