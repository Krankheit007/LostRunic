/**
 * @file LRGuardStateTreeNodes.h
 * @brief 实现“家”垂直切片的守卫感知、0-11 警戒值、StateTree 行为切换、调查追逐与捕获死亡流程。规则层只计算状态，Controller 负责接入 UE 感知、导航和计时器。
 *
 * 关联文件：LRGuardStateTreeNodes.cpp；所属领域：AI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "AI/LRGuardTypes.h"
#include "StateTreeConditionBase.h"
#include "StateTreeTaskBase.h"

#include "LRGuardStateTreeNodes.generated.h"

class ALRGuardAIController;

USTRUCT()
struct FLRGuardBehaviorTaskInstanceData
{
	GENERATED_BODY()

	/** AIController 的领域数据，由所属类型负责维护和校验。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<ALRGuardAIController> AIController;
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
USTRUCT(meta = (DisplayName = "Run Guard Behavior", Category = "Lost Runic|AI"))
struct LOSTRUNIC_API FLRGuardBehaviorTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLRGuardBehaviorTaskInstanceData;
	/**
	 * @brief 查询 Instance Data Type；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 */
	FLRGuardBehaviorTask();
	/**
	 * @brief StateTree 进入节点时让守卫 Controller 启动 Behavior 对应的巡逻、观察、调查、搜索或追逐行为。
	 * @param context 当前 StateTree 执行上下文，用于读取 FLRGuardBehaviorTaskInstanceData。
	 * @param transition 触发本次进入的状态转换结果，供 StateTree 生命周期保持一致。
	 * @return 返回 Running，使行为持续到 StateTree 条件触发下一次转换。
	 */
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context,
		const FStateTreeTransitionResult& transition) const override;
	/**
	 * @brief StateTree 离开节点时通知守卫 Controller 清理 Behavior 拥有的导航、焦点和搜索计时器。
	 * @param context 当前 StateTree 执行上下文，用于取得同一实例数据。
	 * @param transition 触发本次退出的状态转换结果。
	 */
	virtual void ExitState(FStateTreeExecutionContext& context,
		const FStateTreeTransitionResult& transition) const override;

	/** Behavior 的开关；true 表示启用，false 表示禁用。 C++ 安全默认值为 `ELRGuardBehaviorState::IdlePatrol`。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, Category = "Behavior")
	ELRGuardBehaviorState Behavior = ELRGuardBehaviorState::IdlePatrol;
};

USTRUCT()
struct FLRGuardStateConditionInstanceData
{
	GENERATED_BODY()

	/** AIController 的领域数据，由所属类型负责维护和校验。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<ALRGuardAIController> AIController;
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
USTRUCT(meta = (DisplayName = "Guard Behavior Is", Category = "Lost Runic|AI"))
struct LOSTRUNIC_API FLRGuardStateCondition : public FStateTreeConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLRGuardStateConditionInstanceData;
	/**
	 * @brief 查询 Instance Data Type；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	/**
	 * @brief 比较当前守卫行为与 StateTree 条件配置，决定该分支是否可进入。
	 * @param context 用于本次条件匹配的 `context` 标签或上下文。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	virtual bool TestCondition(FStateTreeExecutionContext& context) const override;

	/** Expected Behavior 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `ELRGuardBehaviorState::IdlePatrol`。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, Category = "Behavior")
	ELRGuardBehaviorState ExpectedBehavior = ELRGuardBehaviorState::IdlePatrol;
};
