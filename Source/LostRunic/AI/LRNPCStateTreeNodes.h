/**
 * @file LRNPCStateTreeNodes.h
 * @brief 通用 NPC 的 StateTree 节点：行为任务/条件（执行控制器解析结果）、Idle 玩家朝向任务与限时噪声反应任务；计时器由控制器持有，无 Tick。
 *
 * 关联文件：LRNPCStateTreeNodes.cpp；所属领域：AI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "AI/LRNPCTypes.h"
#include "StateTreeConditionBase.h"
#include "StateTreeTaskBase.h"

#include "LRNPCStateTreeNodes.generated.h"

class ALRNPCController;

USTRUCT()
struct FLRNPCControllerInstanceData
{
	GENERATED_BODY()

	/** AIController 的领域数据，由所属类型负责维护和校验。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<ALRNPCController> AIController;
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
USTRUCT(meta = (DisplayName = "Run NPC Behavior", Category = "Lost Runic|AI"))
struct LOSTRUNIC_API FLRNPCBehaviorTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLRNPCControllerInstanceData;
	/**
	 * @brief 查询 Instance Data Type；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 */
	FLRNPCBehaviorTask();
	/**
	 * @brief StateTree 进入节点时让 NPC 控制器进入配置的行为状态。
	 * @param context 当前 StateTree 执行上下文，用于读取实例数据。
	 * @param transition 触发本次进入的状态转换结果。
	 * @return 返回 Running，使行为持续到 StateTree 条件触发下一次转换。
	 */
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context,
		const FStateTreeTransitionResult& transition) const override;
	/**
	 * @brief StateTree 离开节点时通知 NPC 控制器清理行为拥有的导航、焦点和计时器。
	 * @param context 当前 StateTree 执行上下文。
	 * @param transition 触发本次退出的状态转换结果。
	 */
	virtual void ExitState(FStateTreeExecutionContext& context,
		const FStateTreeTransitionResult& transition) const override;

	/** Behavior 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `ELRNPCBehaviorState::Idle`。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, Category = "Behavior")
	ELRNPCBehaviorState Behavior = ELRNPCBehaviorState::Idle;
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
USTRUCT(meta = (DisplayName = "NPC Behavior Is", Category = "Lost Runic|AI"))
struct LOSTRUNIC_API FLRNPCStateCondition : public FStateTreeConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLRNPCControllerInstanceData;
	/**
	 * @brief 查询 Instance Data Type；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	/**
	 * @brief 比较当前 NPC 行为与 StateTree 条件配置，决定该分支是否可进入；只执行控制器解析结果。
	 * @param context 用于本次条件匹配的 `context` 标签或上下文。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	virtual bool TestCondition(FStateTreeExecutionContext& context) const override;

	/** Expected Behavior 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `ELRNPCBehaviorState::Idle`。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, Category = "Behavior")
	ELRNPCBehaviorState ExpectedBehavior = ELRNPCBehaviorState::Idle;
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
USTRUCT(meta = (DisplayName = "NPC Look At Player", Category = "Lost Runic|AI"))
struct LOSTRUNIC_API FLRNPCLookAtPlayerTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLRNPCControllerInstanceData;
	/**
	 * @brief 查询 Instance Data Type；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 */
	FLRNPCLookAtPlayerTask();
	/**
	 * @brief 进入 Idle 时启动低频玩家朝向检测计时器。
	 * @param context 当前 StateTree 执行上下文。
	 * @param transition 触发本次进入的状态转换结果。
	 * @return 返回 Running，使检测持续到离开 Idle。
	 */
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context,
		const FStateTreeTransitionResult& transition) const override;
	/**
	 * @brief 离开 Idle 时停止朝向检测计时器。
	 * @param context 当前 StateTree 执行上下文。
	 * @param transition 触发本次退出的状态转换结果。
	 */
	virtual void ExitState(FStateTreeExecutionContext& context,
		const FStateTreeTransitionResult& transition) const override;
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
USTRUCT(meta = (DisplayName = "NPC React To Noise", Category = "Lost Runic|AI"))
struct LOSTRUNIC_API FLRNPCReactToNoiseTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLRNPCControllerInstanceData;
	/**
	 * @brief 查询 Instance Data Type；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 */
	FLRNPCReactToNoiseTask();
	/**
	 * @brief 进入 ReactToNoise 时启动限时反应（转向声源，到时发送 NPCReactionEnded）。
	 * @param context 当前 StateTree 执行上下文。
	 * @param transition 触发本次进入的状态转换结果。
	 * @return 返回 Running，使反应持续到超时事件。
	 */
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context,
		const FStateTreeTransitionResult& transition) const override;
	/**
	 * @brief 离开 ReactToNoise 时停止反应计时器。
	 * @param context 当前 StateTree 执行上下文。
	 * @param transition 触发本次退出的状态转换结果。
	 */
	virtual void ExitState(FStateTreeExecutionContext& context,
		const FStateTreeTransitionResult& transition) const override;
};
