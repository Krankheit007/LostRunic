/**
 * @file LRNPCStateTreeNodes.cpp
 * @brief 通用 NPC 的 StateTree 节点实现：行为任务/条件、玩家朝向任务与限时噪声反应任务。
 *
 * 关联文件：LRNPCStateTreeNodes.h；所属领域：AI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "AI/LRNPCStateTreeNodes.h"

#include "AI/LRNPCController.h"
#include "StateTreeExecutionContext.h"

/**
 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
 */
FLRNPCBehaviorTask::FLRNPCBehaviorTask()
{
	bShouldCallTick = false;
}

/**
 * @brief StateTree 进入节点时让 NPC 控制器进入配置的行为状态。
 * @param context 当前 StateTree 执行上下文，用于读取实例数据。
 * @param transition 触发本次进入的状态转换结果。
 * @return 返回 Running，使行为持续到 StateTree 条件触发下一次转换。
 */
EStateTreeRunStatus FLRNPCBehaviorTask::EnterState(FStateTreeExecutionContext& context,
	const FStateTreeTransitionResult& transition) const
{
	const FInstanceDataType& data = context.GetInstanceData(*this);
	if (!data.AIController)
	{
		return EStateTreeRunStatus::Failed;
	}
	data.AIController->EnterBehavior(Behavior);
	return EStateTreeRunStatus::Running;
}

/**
 * @brief StateTree 离开节点时通知 NPC 控制器清理行为拥有的导航、焦点和计时器。
 * @param context 当前 StateTree 执行上下文。
 * @param transition 触发本次退出的状态转换结果。
 */
void FLRNPCBehaviorTask::ExitState(FStateTreeExecutionContext& context,
	const FStateTreeTransitionResult& transition) const
{
	const FInstanceDataType& data = context.GetInstanceData(*this);
	if (data.AIController)
	{
		data.AIController->ExitBehavior(Behavior);
	}
}

/**
 * @brief 比较当前 NPC 行为与 StateTree 条件配置，决定该分支是否可进入；只执行控制器解析结果。
 * @param context 用于本次条件匹配的 `context` 标签或上下文。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool FLRNPCStateCondition::TestCondition(FStateTreeExecutionContext& context) const
{
	const FInstanceDataType& data = context.GetInstanceData(*this);
	return data.AIController && data.AIController->GetActiveBehavior() == ExpectedBehavior;
}

/**
 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
 */
FLRNPCLookAtPlayerTask::FLRNPCLookAtPlayerTask()
{
	bShouldCallTick = false;
}

/**
 * @brief 进入 Idle 时启动低频玩家朝向检测计时器。
 * @param context 当前 StateTree 执行上下文。
 * @param transition 触发本次进入的状态转换结果。
 * @return 返回 Running，使检测持续到离开 Idle。
 */
EStateTreeRunStatus FLRNPCLookAtPlayerTask::EnterState(FStateTreeExecutionContext& context,
	const FStateTreeTransitionResult& transition) const
{
	const FInstanceDataType& data = context.GetInstanceData(*this);
	if (!data.AIController)
	{
		return EStateTreeRunStatus::Failed;
	}
	data.AIController->StartLookAtTimer();
	return EStateTreeRunStatus::Running;
}

/**
 * @brief 离开 Idle 时停止朝向检测计时器。
 * @param context 当前 StateTree 执行上下文。
 * @param transition 触发本次退出的状态转换结果。
 */
void FLRNPCLookAtPlayerTask::ExitState(FStateTreeExecutionContext& context,
	const FStateTreeTransitionResult& transition) const
{
	const FInstanceDataType& data = context.GetInstanceData(*this);
	if (data.AIController)
	{
		data.AIController->StopLookAtTimer();
	}
}

/**
 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
 */
FLRNPCReactToNoiseTask::FLRNPCReactToNoiseTask()
{
	bShouldCallTick = false;
}

/**
 * @brief 进入 ReactToNoise 时启动限时反应（转向声源，到时发送 NPCReactionEnded）。
 * @param context 当前 StateTree 执行上下文。
 * @param transition 触发本次进入的状态转换结果。
 * @return 返回 Running，使反应持续到超时事件。
 */
EStateTreeRunStatus FLRNPCReactToNoiseTask::EnterState(FStateTreeExecutionContext& context,
	const FStateTreeTransitionResult& transition) const
{
	const FInstanceDataType& data = context.GetInstanceData(*this);
	if (!data.AIController)
	{
		return EStateTreeRunStatus::Failed;
	}
	data.AIController->StartNoiseReaction(data.AIController->GetLastNoiseLocation());
	return EStateTreeRunStatus::Running;
}

/**
 * @brief 离开 ReactToNoise 时停止反应计时器。
 * @param context 当前 StateTree 执行上下文。
 * @param transition 触发本次退出的状态转换结果。
 */
void FLRNPCReactToNoiseTask::ExitState(FStateTreeExecutionContext& context,
	const FStateTreeTransitionResult& transition) const
{
	const FInstanceDataType& data = context.GetInstanceData(*this);
	if (data.AIController)
	{
		data.AIController->StopNoiseReaction();
	}
}
