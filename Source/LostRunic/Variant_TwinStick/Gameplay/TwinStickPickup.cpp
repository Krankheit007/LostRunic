// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @file TwinStickPickup.cpp
 * @brief 保留 Unreal TwinStick 模板玩法，用于回归和 PIE 冒烟；它与 /Game/LostRunic 的“家”切片相互独立，不承载 LostRunic 核心叙事规则。
 *
 * 关联文件：TwinStickPickup.h；所属领域：Variant_TwinStick。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */


#include "TwinStickPickup.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "TwinStickCharacter.h"
#include "Components/StaticMeshComponent.h"

/**
 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
 */
ATwinStickPickup::ATwinStickPickup()
{
 	PrimaryActorTick.bCanEverTick = true;

	// create the root component
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// create the collision sphere
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Collision Sphere"));
	CollisionSphere->SetupAttachment(RootComponent);

	CollisionSphere->SetSphereRadius(100.0f);
	CollisionSphere->SetRelativeLocation(FVector(0.0f, 0.0f, 125.0f));
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// create the mesh
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(CollisionSphere);

	Mesh->SetCollisionProfileName(FName("NoCollision"));

}

/**
 * @brief 实现 Notify Actor Begin Overlap 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 * @param OtherActor 参与本次操作的运行时对象 `OtherActor`；函数会检查空值和所需接口。
 */
void ATwinStickPickup::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	// have we overlapped the player character?
	if (ATwinStickCharacter* PlayerCharacter = Cast<ATwinStickCharacter>(OtherActor))
	{
		// give the pickup to the player
		PlayerCharacter->AddPickup();

		// destroy this pickup
		Destroy();
	}
}
