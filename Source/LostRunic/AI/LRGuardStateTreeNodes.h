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

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<ALRGuardAIController> AIController;
};

/** Enters and exits one controller-owned guard behavior without polling. */
USTRUCT(meta = (DisplayName = "Run Guard Behavior", Category = "Lost Runic|AI"))
struct LOSTRUNIC_API FLRGuardBehaviorTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLRGuardBehaviorTaskInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	FLRGuardBehaviorTask();
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context,
		const FStateTreeTransitionResult& transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& context,
		const FStateTreeTransitionResult& transition) const override;

	UPROPERTY(EditAnywhere, Category = "Behavior")
	ELRGuardBehaviorState Behavior = ELRGuardBehaviorState::IdlePatrol;
};

USTRUCT()
struct FLRGuardStateConditionInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<ALRGuardAIController> AIController;
};

/** Pure StateTree condition matching the controller's current alert-derived behavior. */
USTRUCT(meta = (DisplayName = "Guard Behavior Is", Category = "Lost Runic|AI"))
struct LOSTRUNIC_API FLRGuardStateCondition : public FStateTreeConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLRGuardStateConditionInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& context) const override;

	UPROPERTY(EditAnywhere, Category = "Behavior")
	ELRGuardBehaviorState ExpectedBehavior = ELRGuardBehaviorState::IdlePatrol;
};
