// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @file TwinStickStateTreeUtility.cpp
 * @brief 保留 Unreal TwinStick 模板玩法，用于回归和 PIE 冒烟；它与 /Game/LostRunic 的“家”切片相互独立，不承载 LostRunic 核心叙事规则。
 *
 * 关联文件：TwinStickStateTreeUtility.h；所属领域：Variant_TwinStick。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */


#include "TwinStickStateTreeUtility.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeExecutionTypes.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

#define LOCTEXT_NAMESPACE "TopDownTemplate"

/**
 * @brief 实现 Tick 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 * @param Context 用于本次条件匹配的 `Context` 标签或上下文。
 * @param DeltaTime 时间值 `DeltaTime`，单位为秒。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
EStateTreeRunStatus FStateTreeGetPlayerTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	// get the instance data
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	// get the pawn possessed by the first local player
	InstanceData.TargetPlayerCharacter = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(InstanceData.Character, 0));

	// keep the task running
	return EStateTreeRunStatus::Running;
}

#if WITH_EDITOR
/**
 * @brief 查询 Description；不修改领域状态。
 * @param ID 稳定标识 `ID`；用于内容查询和存档，不依赖显示名或数组序号。
 * @param InstanceDataView 调用方提供的 `InstanceDataView`，只在本次操作范围内使用。
 * @param BindingLookup 调用方提供的 `BindingLookup`，只在本次操作范围内使用。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FText FStateTreeGetPlayerTask::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting /*= EStateTreeNodeFormatting::Text*/) const
{
	return LOCTEXT("StateTreeTaskGetPlayerDescription", "<b>Get Player</b>");
}
#endif // WITH_EDITOR
