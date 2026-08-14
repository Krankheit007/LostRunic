/**
 * @file LRGuardStateTreeNodes.cpp
 * @brief 实现“家”垂直切片的守卫感知、0-11 警戒值、StateTree 行为切换、调查追逐与捕获死亡流程。规则层只计算状态，Controller 负责接入 UE 感知、导航和计时器。
 *
 * 关联文件：LRGuardStateTreeNodes.h；所属领域：AI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "AI/LRGuardStateTreeNodes.h"

#include "AI/LRAlertComponent.h"
#include "AI/LRGuardAIController.h"
#include "StateTreeExecutionContext.h"

/**
 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
 */
FLRGuardBehaviorTask::FLRGuardBehaviorTask()
{
	bShouldCallTick = false;
}

/**
 * @brief StateTree 任务进入时通知守卫 Controller 切换到配置的行为状态。
 * @param context 用于本次条件匹配的 `context` 标签或上下文。
 * @param transition 本次领域操作的结构化数据 `transition`；字段语义由对应 USTRUCT 定义。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
EStateTreeRunStatus FLRGuardBehaviorTask::EnterState(FStateTreeExecutionContext& context,
	const FStateTreeTransitionResult& transition) const
{
	FInstanceDataType& data = context.GetInstanceData(*this);
	if (!data.AIController)
	{
		return EStateTreeRunStatus::Failed;
	}
	data.AIController->EnterBehavior(Behavior);
	return EStateTreeRunStatus::Running;
}

/**
 * @brief StateTree 任务退出时通知守卫 Controller 清理该行为拥有的导航和计时器。
 * @param context 用于本次条件匹配的 `context` 标签或上下文。
 * @param transition 本次领域操作的结构化数据 `transition`；字段语义由对应 USTRUCT 定义。
 */
void FLRGuardBehaviorTask::ExitState(FStateTreeExecutionContext& context,
	const FStateTreeTransitionResult& transition) const
{
	const FInstanceDataType& data = context.GetInstanceData(*this);
	if (data.AIController)
	{
		data.AIController->ExitBehavior(Behavior);
	}
}

/**
 * @brief 比较当前守卫行为与 StateTree 条件配置，决定该分支是否可进入。
 * @param context 用于本次条件匹配的 `context` 标签或上下文。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool FLRGuardStateCondition::TestCondition(FStateTreeExecutionContext& context) const
{
	const FInstanceDataType& data = context.GetInstanceData(*this);
	// StateTree 只执行控制器解析的结果，不自行重新定义警戒语义。
	return data.AIController && data.AIController->GetResolvedBehavior() == ExpectedBehavior;
}
