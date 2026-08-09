// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @file TwinStickSpawner.cpp
 * @brief 保留 Unreal TwinStick 模板玩法，用于回归和 PIE 冒烟；它与 /Game/LostRunic 的“家”切片相互独立，不承载 LostRunic 核心叙事规则。
 *
 * 关联文件：TwinStickSpawner.h；所属领域：Variant_TwinStick。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */


#include "TwinStickSpawner.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "NavigationSystem.h"
#include "NavMesh/RecastNavMesh.h"
#include "Kismet/GameplayStatics.h"
#include "TwinStickNPC.h"
#include "TwinStickGameMode.h"

/**
 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
 */
ATwinStickSpawner::ATwinStickSpawner()
{
 	PrimaryActorTick.bCanEverTick = true;

}

/**
 * @brief 在进入世界后解析运行时依赖、绑定事件并启动所需计时器；构造阶段不访问 World 或玩家对象。
 */
void ATwinStickSpawner::BeginPlay()
{
	Super::BeginPlay();

	// find the recast navmesh actor on the level
	TArray<AActor*> ActorList;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARecastNavMesh::StaticClass(), ActorList);

	if (ActorList.Num() > 0)
	{
		NavData = Cast<ARecastNavMesh>(ActorList[0]);
	} else {

		UE_LOG(LogTemp, Log, TEXT("Could not find recast navmesh"));

	}

	// set up the spawn timer
	GetWorld()->GetTimerManager().SetTimer(SpawnGroupTimer, this, &ATwinStickSpawner::SpawnNPCGroup, SpawnGroupDelay, true);

	// spawn the first group of NPCs
	SpawnNPCGroup();
}

/**
 * @brief 解除委托并清理计时器或缓存，避免关卡切换和对象销毁后继续收到回调。
 * @param EndPlayReason Unreal 提供的结束原因，用于区分销毁、关卡切换和退出。
 */
void ATwinStickSpawner::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// clear the spawn timers
	GetWorld()->GetTimerManager().ClearTimer(SpawnGroupTimer);
	GetWorld()->GetTimerManager().ClearTimer(SpawnNPCTimer);
}

/**
 * @brief 实现 Spawn NPCGroup 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 */
void ATwinStickSpawner::SpawnNPCGroup()
{
	// reset the group spawn counter
	SpawnCount = 0;

	// check if we're still under the max NPC cap
	if (ATwinStickGameMode* GM = Cast<ATwinStickGameMode>(GetWorld()->GetAuthGameMode()))
	{
		if (GM->CanSpawnNPCs())
		{
			SpawnNPC();
		}
	}
}

/**
 * @brief 实现 Spawn NPC 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 */
void ATwinStickSpawner::SpawnNPC()
{
	FTransform SpawnTransform;

	// find a random point around the spawner
	FVector SpawnLoc;
	if (UNavigationSystemV1::K2_GetRandomReachablePointInRadius(GetWorld(), GetActorLocation(), SpawnLoc, SpawnRadius, NavData))
	{
		SpawnTransform.SetLocation(SpawnLoc);

		// spawn the NPC
		ATwinStickNPC* NPC = GetWorld()->SpawnActor<ATwinStickNPC>(NPCClass, SpawnTransform);
	}

	// increase the spawn counter
	++SpawnCount;

	// do we still have enemies left to spawn?
	if (SpawnCount < SpawnGroupSize)
	{
		GetWorld()->GetTimerManager().SetTimer(SpawnNPCTimer, this, &ATwinStickSpawner::SpawnNPC, FMath::RandRange(MinSpawnDelay, MaxSpawnDelay), false);
	}

}
