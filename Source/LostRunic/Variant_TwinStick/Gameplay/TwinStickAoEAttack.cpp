// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @file TwinStickAoEAttack.cpp
 * @brief 保留 Unreal TwinStick 模板玩法，用于回归和 PIE 冒烟；它与 /Game/LostRunic 的“家”切片相互独立，不承载 LostRunic 核心叙事规则。
 *
 * 关联文件：TwinStickAoEAttack.h；所属领域：Variant_TwinStick。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */


#include "TwinStickAoEAttack.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "TwinStickNPC.h"

/**
 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
 */
ATwinStickAoEAttack::ATwinStickAoEAttack()
{
 	PrimaryActorTick.bCanEverTick = true;

	// create the root component
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// create the mesh that provides the visual representation for the AoE
	SphereVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Sphere Visual"));
	SphereVisual->SetupAttachment(RootComponent);

	SphereVisual->SetCollisionProfileName(FName("NoCollision"));

	// create the collision sphere
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Collision Sphere"));
	CollisionSphere->SetupAttachment(RootComponent);

	CollisionSphere->SetSphereRadius(750.0f);
	CollisionSphere->SetNotifyRigidBodyCollision(true);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ATwinStickAoEAttack::OnAoEOverlap);
}

/**
 * @brief 在进入世界后解析运行时依赖、绑定事件并启动所需计时器；构造阶段不访问 World 或玩家对象。
 */
void ATwinStickAoEAttack::BeginPlay()
{
	Super::BeginPlay();

	// set up the AoE timers
	GetWorld()->GetTimerManager().SetTimer(StartAoETimer, this, &ATwinStickAoEAttack::StartAoE, StartAoETime, false);
	GetWorld()->GetTimerManager().SetTimer(StopAoETimer, this, &ATwinStickAoEAttack::StopAoE, StopAoETime, false);

}

/**
 * @brief 解除委托并清理计时器或缓存，避免关卡切换和对象销毁后继续收到回调。
 * @param EndPlayReason Unreal 提供的结束原因，用于区分销毁、关卡切换和退出。
 */
void ATwinStickAoEAttack::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// clear the timers
	GetWorld()->GetTimerManager().ClearTimer(StartAoETimer);
	GetWorld()->GetTimerManager().ClearTimer(StopAoETimer);
}

/**
 * @brief 开始 Start Ao E 流程，建立本次操作拥有的状态、委托或计时器。
 */
void ATwinStickAoEAttack::StartAoE()
{
	// raise the active flag
	bIsAoEActive = true;

	// find all actors overlapping the NPC
	TArray<AActor*> Overlaps;
	CollisionSphere->GetOverlappingActors(Overlaps, ATwinStickNPC::StaticClass());

	// process each overlapping actor
	for (AActor* Current : Overlaps)
	{
		if (ATwinStickNPC* NPC = Cast<ATwinStickNPC>(Current))
		{
			// tell the NPC it's been hit
			NPC->ProjectileImpact(FVector::ZeroVector);
		}
	}
}

/**
 * @brief 结束或取消 Stop Ao E 流程，并清理本次操作拥有的临时状态。
 */
void ATwinStickAoEAttack::StopAoE()
{
	// drop the active flag
	bIsAoEActive = false;

	// stop the damage tick timer
	GetWorld()->GetTimerManager().ClearTimer(StartAoETimer);

	// call the BP handler. It will be responsible for destroying the Actor when it's done
	BP_AoEFinished();
}

/**
 * @brief 处理 On Ao EOverlap 事件，将引擎回调转换为对应领域状态更新。
 * @param OverlappedComponent 参与本次操作的运行时对象 `OverlappedComponent`；函数会检查空值和所需接口。
 * @param OtherActor 参与本次操作的运行时对象 `OtherActor`；函数会检查空值和所需接口。
 * @param OtherComp 调用方提供的 `OtherComp`，只在本次操作范围内使用。
 * @param OtherBodyIndex 本次操作使用的计数、增量或索引 `OtherBodyIndex`；由函数校验合法范围。
 * @param bFromSweep 布尔开关 `bFromSweep`；true 表示启用或条件成立，false 表示禁用或条件不成立。
 * @param SweepResult 本次领域操作的结构化数据 `SweepResult`；字段语义由对应 USTRUCT 定义。
 */
void ATwinStickAoEAttack::OnAoEOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// is the explosion active?
	if (bIsAoEActive)
	{
		// did we overlap an NPC?
		if (ATwinStickNPC* NPC = Cast<ATwinStickNPC>(OtherActor))
		{
			// tell the NPC it's been hit
			NPC->ProjectileImpact(FVector::ZeroVector);
		}
	}
}
