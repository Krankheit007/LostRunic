#include "AI/LRGuardStateTreeNodes.h"

#include "AI/LRAlertComponent.h"
#include "AI/LRGuardAIController.h"
#include "StateTreeExecutionContext.h"

FLRGuardBehaviorTask::FLRGuardBehaviorTask()
{
	bShouldCallTick = false;
}

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

void FLRGuardBehaviorTask::ExitState(FStateTreeExecutionContext& context,
	const FStateTreeTransitionResult& transition) const
{
	const FInstanceDataType& data = context.GetInstanceData(*this);
	if (data.AIController)
	{
		data.AIController->ExitBehavior(Behavior);
	}
}

bool FLRGuardStateCondition::TestCondition(FStateTreeExecutionContext& context) const
{
	const FInstanceDataType& data = context.GetInstanceData(*this);
	const ULRAlertComponent* alert = data.AIController ? data.AIController->GetAlertComponent() : nullptr;
	return alert && alert->GetBehaviorState() == ExpectedBehavior;
}
