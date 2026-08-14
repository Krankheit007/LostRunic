/**
 * @file LRGuardCharacter.cpp
 * @brief 实现“家”垂直切片的守卫感知、0-11 警戒值、StateTree 行为切换、调查追逐与捕获死亡流程。规则层只计算状态，Controller 负责接入 UE 感知、导航和计时器。
 *
 * 关联文件：LRGuardCharacter.h；所属领域：AI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "AI/LRGuardCharacter.h"

#include "AI/LRAlertComponent.h"
#include "AI/LRGuardAIController.h"
#include "Components/WidgetComponent.h"
#include "Core/LRGameplayTags.h"
#include "Engine/GameInstance.h"
#include "Framework/LRCharacter.h"
#include "Items/LRCourageResponseComponent.h"
#include "Save/LRSaveSubsystem.h"
#include "State/LRStateComponent.h"
#include "UI/LRWorldAlertBarWidgetBase.h"

/**
 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
 */
ALRGuardCharacter::ALRGuardCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	AIControllerClass = ALRGuardAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	Alert = CreateDefaultSubobject<ULRAlertComponent>(TEXT("Alert"));
	CourageResponse = CreateDefaultSubobject<ULRCourageResponseComponent>(TEXT("CourageResponse"));
	AlertWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("AlertWidget"));
	AlertWidget->SetupAttachment(GetMesh());
	AlertWidget->SetWidgetSpace(EWidgetSpace::Screen);
	AlertWidget->SetDrawSize(FVector2D(120.0f, 24.0f));
	AlertWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

/**
 * @brief 在进入世界后解析运行时依赖：将世界警戒条 Widget 初始化到本守卫的警戒快照。
 */
void ALRGuardCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (UUserWidget* widget = AlertWidget->GetWidget())
	{
		if (ULRWorldAlertBarWidgetBase* alertBar = Cast<ULRWorldAlertBarWidgetBase>(widget))
		{
			alertBar->InitializeForGuard(this);
		}
	}
}

/**
 * @brief 确认目标仍可捕获后提交死亡状态请求，并启动死亡到 Memory 的存档事务。
 * @param target 本次规则检查或操作的目标对象。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ALRGuardCharacter::CaptureTarget(AActor* target)
{
	ULRStateComponent* state = target ? target->FindComponentByClass<ULRStateComponent>() : nullptr;
	if (!state)
	{
		return false;
	}
	FLRStateChangeRequest request;
	request.TargetMode = ELRPerceptionMode::Memory;
	request.RequestType = ELRStateRequestType::Death;
	request.Source = LRGameplayTags::StateSourceDeath;
	const FLRStateChangeResult result = state->RequestStateChange(request);
	if (result.bAccepted)
	{
		if (ALRCharacter* character = Cast<ALRCharacter>(target))
		{
			if (UGameInstance* gameInstance = character->GetGameInstance())
			{
				if (ULRSaveSubsystem* saveSubsystem = gameInstance->GetSubsystem<ULRSaveSubsystem>())
				{
					saveSubsystem->BeginDeathMemoryTransaction(character);
				}
			}
		}
		OnPlayerCaptured.Broadcast(target);
	}
	return result.bAccepted;
}

/**
 * @brief 查询 Patrol Point；不修改领域状态。
 * @param index 目标元素索引，调用前必须满足对应容器边界。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
AActor* ALRGuardCharacter::GetPatrolPoint(const int32 index) const
{
	return PatrolPoints.IsValidIndex(index) ? PatrolPoints[index] : nullptr;
}
