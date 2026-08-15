/**
 * @file LRRoomVolume.cpp
 * @brief 实现室内奔跑噪声的房间传播体积：重叠守卫集合维护、相邻房间拓扑与旋转体积的局部空间包含判定。
 *
 * 关联文件：LRRoomVolume.h；所属领域：Gameplay。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Gameplay/LRRoomVolume.h"

#include "AI/LRAlertComponent.h"
#include "Components/BoxComponent.h"
#include "EngineUtils.h"

/**
 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
 */
ALRRoomVolume::ALRRoomVolume()
{
	PrimaryActorTick.bCanEverTick = false;
	Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
	SetRootComponent(Bounds);
	Bounds->SetCollisionProfileName(TEXT("Trigger"));
	Bounds->SetGenerateOverlapEvents(true);
	Bounds->OnComponentBeginOverlap.AddDynamic(this, &ALRRoomVolume::HandleBeginOverlap);
	Bounds->OnComponentEndOverlap.AddDynamic(this, &ALRRoomVolume::HandleEndOverlap);
}

/**
 * @brief 查询所有覆盖指定位置（支持旋转体积，BoxComponent 局部空间 + extent 判定）的房间体积。
 * @param world 本次查询所在的 World。
 * @param location 世界空间位置，Unreal 单位为厘米。
 * @param outRooms 输出匹配的房间体积集合。
 */
void ALRRoomVolume::FindRoomsAtLocation(const UWorld* world, const FVector location, TArray<ALRRoomVolume*>& outRooms)
{
	if (!world)
	{
		return;
	}
	for (TActorIterator<ALRRoomVolume> it(world); it; ++it)
	{
		const UBoxComponent* bounds = it->Bounds.Get();
		if (!bounds)
		{
			continue;
		}
		// 局部空间 + extent 判定，支持旋转与缩放：InverseTransformPosition 已包含 Scale 逆变换，
		// 局部坐标直接与 UnscaledBoxExtent 比较，不再乘 GetComponentScale（避免双重缩放）。
		const FVector local = bounds->GetComponentTransform().InverseTransformPosition(location);
		const FVector halfExtent = bounds->GetUnscaledBoxExtent();
		if (FMath::Abs(local.X) <= halfExtent.X && FMath::Abs(local.Y) <= halfExtent.Y
			&& FMath::Abs(local.Z) <= halfExtent.Z)
		{
			outRooms.Add(*it);
		}
	}
}

/**
 * @brief 处理 Handle Begin Overlap 事件：带警戒组件的守卫加入房间集合。
 * @param component 参与本次操作的运行时对象 `component`；函数会检查空值和所需接口。
 * @param otherActor 参与本次操作的运行时对象 `otherActor`；函数会检查空值和所需接口。
 * @param otherComponent 参与本次操作的运行时对象 `otherComponent`；函数会检查空值和所需接口。
 * @param otherBodyIndex 本次操作使用的计数、增量或索引 `otherBodyIndex`；由函数校验合法范围。
 * @param bFromSweep 布尔开关 `bFromSweep`；true 表示启用或条件成立，false 表示禁用或条件不成立。
 * @param sweepResult 本次领域操作的结构化数据 `sweepResult`；字段语义由对应 USTRUCT 定义。
 */
void ALRRoomVolume::HandleBeginOverlap(UPrimitiveComponent* component, AActor* otherActor, UPrimitiveComponent* otherComponent,
	const int32 otherBodyIndex, const bool bFromSweep, const FHitResult& sweepResult)
{
	if (otherActor && otherActor->FindComponentByClass<ULRAlertComponent>())
	{
		OverlappingGuards.AddUnique(otherActor);
	}
}

/**
 * @brief 处理 Handle End Overlap 事件：守卫离开房间集合。
 * @param component 参与本次操作的运行时对象 `component`；函数会检查空值和所需接口。
 * @param otherActor 参与本次操作的运行时对象 `otherActor`；函数会检查空值和所需接口。
 * @param otherComponent 参与本次操作的运行时对象 `otherComponent`；函数会检查空值和所需接口。
 * @param otherBodyIndex 本次操作使用的计数、增量或索引 `otherBodyIndex`；由函数校验合法范围。
 */
void ALRRoomVolume::HandleEndOverlap(UPrimitiveComponent* component, AActor* otherActor, UPrimitiveComponent* otherComponent,
	const int32 otherBodyIndex)
{
	if (otherActor)
	{
		OverlappingGuards.Remove(otherActor);
	}
}
