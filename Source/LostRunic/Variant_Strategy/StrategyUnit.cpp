// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @file StrategyUnit.cpp
 * @brief 保留 Unreal Strategy 模板玩法，用于回归和 PIE 冒烟；它与 /Game/LostRunic 的“家”切片相互独立，不承载 LostRunic 核心叙事规则。
 *
 * 关联文件：StrategyUnit.h；所属领域：Variant_Strategy。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */


#include "StrategyUnit.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/SphereComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "EnvironmentQuery/EnvQueryInstanceBlueprintWrapper.h"
#include "Engine/OverlapResult.h"

/**
 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
 */
AStrategyUnit::AStrategyUnit()
{
	PrimaryActorTick.bCanEverTick = true;

	// ensure this unit has a valid AI controller to handle move requests
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// create the interaction range sphere
	InteractionRange = CreateDefaultSubobject<USphereComponent>(TEXT("Interaction Range"));
	InteractionRange->SetupAttachment(RootComponent);

	InteractionRange->SetSphereRadius(100.0f);
	InteractionRange->SetCollisionProfileName(FName("OverlapAllDynamic"));

	// configure movement
	GetCharacterMovement()->GravityScale = 1.5f;
	GetCharacterMovement()->MaxAcceleration = 1000.0f;
	GetCharacterMovement()->BrakingFrictionFactor = 1.0f;
	GetCharacterMovement()->BrakingDecelerationWalking = 1000.0f;
	GetCharacterMovement()->PerchRadiusThreshold = 20.0f;
	GetCharacterMovement()->bUseFlatBaseForFloorChecks = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 640.0f, 0.0f);
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->AvoidanceConsiderationRadius = 150.0f;
	GetCharacterMovement()->AvoidanceWeight = 1.0f;
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;
	GetCharacterMovement()->SetFixedBrakingDistance(200.0f);
	GetCharacterMovement()->SetFixedBrakingDistance(true);
}

/**
 * @brief 实现 Notify Controller Changed 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 */
void AStrategyUnit::NotifyControllerChanged()
{
	// validate and save a copy of the AI controller reference
	AIController = Cast<AAIController>(Controller);

	if (AIController)
	{
		// subscribe to the move finished handler on the path following component
		UPathFollowingComponent* PFComp = AIController->GetPathFollowingComponent();
		if (PFComp)
		{
			PFComp->OnRequestFinished.AddUObject(this, &AStrategyUnit::OnMoveFinished);
		}
	}
}

/**
 * @brief 结束或取消 Stop Moving 流程，并清理本次操作拥有的临时状态。
 */
void AStrategyUnit::StopMoving()
{
	// use the character movement component to stop movement
	GetCharacterMovement()->StopMovementImmediately();

	// stop the unit's interaction animation
	BP_StopAnimation();
}

/**
 * @brief 实现 Unit Selected 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 */
void AStrategyUnit::UnitSelected()
{
	// pass control to BP
	BP_UnitSelected();
}

/**
 * @brief 实现 Unit Deselected 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 */
void AStrategyUnit::UnitDeselected()
{
	// pass control to BP
	BP_UnitDeselected();
}

/**
 * @brief 执行 Interact 的玩法动作；输入层只提供语义，合法性由对应领域组件决定。
 * @param Interactor 参与本次操作的运行时对象 `Interactor`；函数会检查空值和所需接口。
 */
void AStrategyUnit::Interact(AStrategyUnit* Interactor)
{
	// ensure the interactor is valid
	if (IsValid(Interactor))
	{
		// rotate towards the actor we're interacting with
		SetActorRotation(UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), Interactor->GetActorLocation()));

		// signal the interactor to play its interaction behavior
		Interactor->BP_InteractionBehavior(this);

		// play our own interaction behavior
		BP_InteractionBehavior(Interactor);
	}

}

/**
 * @brief 执行 Move To Location 的玩法动作；输入层只提供语义，合法性由对应领域组件决定。
 * @param Location 世界空间位置，Unreal 单位为厘米。
 * @param bInteract 布尔开关 `bInteract`；true 表示启用或条件成立，false 表示禁用或条件不成立。
 * @param IgnoreList 调用方提供的 `IgnoreList`，只在本次操作范围内使用。
 */
void AStrategyUnit::MoveToLocation(const FVector& Location, bool bInteract, const TArray<AStrategyUnit*> IgnoreList)
{
	// cache the movement and interaction parameters
	CurrentMovementGoal = Location;
	bInteractOnArrival = bInteract;
	InteractIgnoreList = IgnoreList;

	// stop movement and animation
	StopMoving();

	// choose the EnvQuery to use
	UEnvQuery* MoveQuery = bInteractOnArrival ? InteractionQuery : NoInteractionQuery;

	// choose the run mode to use. The main interacting unit gets the closest result, all others choose randomly from top 25%
	TEnumAsByte<EEnvQueryRunMode::Type> RunMode = bInteractOnArrival ? EEnvQueryRunMode::SingleResult : EEnvQueryRunMode::RandomBest25Pct;

	// run an EQS to resolve the movement destination using the NavMesh
	EnvQueryInstance = UEnvQueryManager::RunEQSQuery(this, MoveQuery, this,  RunMode, UEnvQueryInstanceBlueprintWrapper::StaticClass());

	if (IsValid(EnvQueryInstance))
	{
		EnvQueryInstance->GetOnQueryFinishedEvent().AddDynamic(this, &AStrategyUnit::OnEQSFinished);
	}
}

/**
 * @brief 查询 Movement Goal；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FVector AStrategyUnit::GetMovementGoal() const
{
	return CurrentMovementGoal;
}

/**
 * @brief 处理 On EQSFinished 事件，将引擎回调转换为对应领域状态更新。
 * @param QueryInstance 调用方提供的 `QueryInstance`，只在本次操作范围内使用。
 * @param QueryStatus 调用方提供的 `QueryStatus`，只在本次操作范围内使用。
 */
void AStrategyUnit::OnEQSFinished(UEnvQueryInstanceBlueprintWrapper* QueryInstance, EEnvQueryStatus::Type QueryStatus)
{
	// was the EnvQuery successful?
	if (QueryInstance)
	{
		// get the query result locations
		TArray<FVector> ResultLocations;

		if(QueryInstance->GetQueryResultsAsLocations(ResultLocations))
		{
			// grab the top result
			CurrentMovementGoal = ResultLocations[0];

			// ensure we have a valid AI Controller
			if (AIController)
			{
				// set up the AI Move Request
				FAIMoveRequest MoveReq;

				MoveReq.SetGoalLocation(CurrentMovementGoal);
				MoveReq.SetAcceptanceRadius(MovementAcceptanceRadius);
				MoveReq.SetAllowPartialPath(true);
				MoveReq.SetUsePathfinding(true);
				MoveReq.SetProjectGoalLocation(true);
				MoveReq.SetRequireNavigableEndLocation(true);
				MoveReq.SetNavigationFilter(AIController->GetDefaultNavigationFilterClass());
				MoveReq.SetCanStrafe(false);

				// request a move to the AI Controller
				FNavPathSharedPtr FollowedPath;
				const FPathFollowingRequestResult ResultData = AIController->MoveTo(MoveReq, &FollowedPath);

				// check if we're already at the goal
				if(ResultData.Code == EPathFollowingRequestResult::AlreadyAtGoal)
				{
					// finish movement immediately
					HandleMoveFinished();
				}
			}
		}
	}
}

/**
 * @brief 处理 On Move Finished 事件，将引擎回调转换为对应领域状态更新。
 * @param RequestID 稳定标识 `RequestID`；用于内容查询和存档，不依赖显示名或数组序号。
 * @param Result 本次领域操作的结构化数据 `Result`；字段语义由对应 USTRUCT 定义。
 */
void AStrategyUnit::OnMoveFinished(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	HandleMoveFinished();
}

/**
 * @brief 处理 Handle Move Finished 事件，将引擎回调转换为对应领域状态更新。
 */
void AStrategyUnit::HandleMoveFinished()
{
	// broadcast the move completed delegate
	OnMoveCompleted.Broadcast(this);

	if (bInteractOnArrival)
	{
		// do an overlap test to find nearby interactive objects
		TArray<FOverlapResult> OutOverlaps;

		FCollisionShape CollisionSphere;
		CollisionSphere.SetSphere(InteractionRadius);

		FCollisionObjectQueryParams ObjectParams;
		ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

		FCollisionQueryParams QueryParams;

		// add the selected units to the ignored list
		QueryParams.AddIgnoredActor(this);

		for (const AActor* Current : InteractIgnoreList)
		{
			QueryParams.AddIgnoredActor(Current);
		}

		if (GetWorld()->OverlapMultiByObjectType(OutOverlaps, GetActorLocation(), FQuat::Identity, ObjectParams, CollisionSphere, QueryParams))
		{
			// find the first unit we've overlapped, and interact with it
			for (const FOverlapResult& CurrentOverlap : OutOverlaps)
			{
				if (AStrategyUnit* CurrentUnit = Cast<AStrategyUnit>(CurrentOverlap.GetActor()))
				{
					CurrentUnit->Interact(this);
					return;
				}
			}
		}
	}
}
