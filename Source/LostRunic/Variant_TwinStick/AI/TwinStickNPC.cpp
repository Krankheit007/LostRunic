// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @file TwinStickNPC.cpp
 * @brief 保留 Unreal TwinStick 模板玩法，用于回归和 PIE 冒烟；它与 /Game/LostRunic 的“家”切片相互独立，不承载 LostRunic 核心叙事规则。
 *
 * 关联文件：TwinStickNPC.h；所属领域：Variant_TwinStick。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */


#include "TwinStickNPC.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "TwinStickCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TwinStickGameMode.h"
#include "TwinStickPickup.h"
#include "Engine/World.h"
#include "TwinStickNPCDestruction.h"
#include "TimerManager.h"

/**
 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
 */
ATwinStickNPC::ATwinStickNPC()
{
	PrimaryActorTick.bCanEverTick = true;

	// ensure we spawn an AI controller when we're spawned
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// configure the inherited components
	GetCapsuleComponent()->SetCapsuleRadius(45.0f);
	GetCapsuleComponent()->SetNotifyRigidBodyCollision(true);

	GetMesh()->SetCollisionProfileName(FName("NoCollision"));

	GetCharacterMovement()->GravityScale = 1.5f;
	GetCharacterMovement()->MaxAcceleration = 1000.0f;
	GetCharacterMovement()->BrakingFriction = 1.0f;
	GetCharacterMovement()->MaxWalkSpeed = 200.0f;
	GetCharacterMovement()->MaxWalkSpeedCrouched = 100.0f;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 640.0f, 0.0f);
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->bUseRVOAvoidance = true;
	GetCharacterMovement()->AvoidanceConsiderationRadius = 250.0f;
	GetCharacterMovement()->AvoidanceWeight = 1.0f;
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;
}

/**
 * @brief 在进入世界后解析运行时依赖、绑定事件并启动所需计时器；构造阶段不访问 World 或玩家对象。
 */
void ATwinStickNPC::BeginPlay()
{
	Super::BeginPlay();

	// increment the NPC counter so we can cap spawning if necessary
	if (ATwinStickGameMode* GM = Cast<ATwinStickGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->IncreaseNPCs();
	}

}

/**
 * @brief 解除委托并清理计时器或缓存，避免关卡切换和对象销毁后继续收到回调。
 * @param EndPlayReason Unreal 提供的结束原因，用于区分销毁、关卡切换和退出。
 */
void ATwinStickNPC::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// clear the destruction timer
	GetWorld()->GetTimerManager().ClearTimer(DestructionTimer);
}

/**
 * @brief 实现 Destroyed 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 */
void ATwinStickNPC::Destroyed()
{
	// decrease the NPC counter so we can cap spawning if necessary
	if (ATwinStickGameMode* GM = Cast<ATwinStickGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->DecreaseNPCs();
	}

	Super::Destroyed();
}

/**
 * @brief 实现 Notify Hit 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 * @param MyComp 调用方提供的 `MyComp`，只在本次操作范围内使用。
 * @param Other 调用方提供的 `Other`，只在本次操作范围内使用。
 * @param OtherComp 调用方提供的 `OtherComp`，只在本次操作范围内使用。
 * @param bSelfMoved 布尔开关 `bSelfMoved`；true 表示启用或条件成立，false 表示禁用或条件不成立。
 * @param HitLocation 空间值 `HitLocation`；距离和位置使用 Unreal 厘米单位。
 * @param HitNormal 调用方提供的 `HitNormal`，只在本次操作范围内使用。
 * @param NormalImpulse 调用方提供的 `NormalImpulse`，只在本次操作范围内使用。
 * @param Hit 调用方提供的 `Hit`，只在本次操作范围内使用。
 */
void ATwinStickNPC::NotifyHit(class UPrimitiveComponent* MyComp, AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	// have we collided against the player?
	if (ATwinStickCharacter* PlayerCharacter = Cast<ATwinStickCharacter>(Other))
	{
		// apply damage to the character
		PlayerCharacter->HandleDamage(1.0f, GetActorForwardVector());
	}
}

/**
 * @brief 实现 Projectile Impact 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 * @param ForwardVector 调用方提供的 `ForwardVector`，只在本次操作范围内使用。
 */
void ATwinStickNPC::ProjectileImpact(const FVector& ForwardVector)
{
	// only handle damage if we haven't been hit yet
	if (bHit)
	{
		return;
	}

	// raise the hit flag
	bHit = true;

	// deactivate character movement
	GetCharacterMovement()->Deactivate();

	// award points
	if (ATwinStickGameMode* GM = Cast<ATwinStickGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->ScoreUpdate(Score);
	}

	// randomly spawn a pickup
	if (FMath::RandRange(0, 100) < PickupSpawnChance)
	{
		ATwinStickPickup* Pickup = GetWorld()->SpawnActor<ATwinStickPickup>(PickupClass, GetActorTransform());
	}

	// spawn the NPC destruction proxy
	ATwinStickNPCDestruction* DestructionProxy = GetWorld()->SpawnActor<ATwinStickNPCDestruction>(DestructionProxyClass, GetActorTransform());

	// hide this actor
	SetActorHiddenInGame(true);

	// disable collision
	SetActorEnableCollision(false);

	// defer destruction
	GetWorld()->GetTimerManager().SetTimer(DestructionTimer, this, &ATwinStickNPC::DeferredDestroy, DeferredDestructionTime, false);
}

/**
 * @brief 实现 Deferred Destroy 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 */
void ATwinStickNPC::DeferredDestroy()
{
	// destroy this actor
	Destroy();
}
