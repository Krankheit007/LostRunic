/**
 * @file LRNoiseArea.cpp
 * @brief 实现角色移动模式、按移动距离产生脚步和室内外噪声区域等基础玩法能力；区域维护进入/退出集合，重叠按固定优先级解析，无区域时默认 Outdoor。
 *
 * 关联文件：LRNoiseArea.h；所属领域：Gameplay。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Gameplay/LRNoiseArea.h"

#include "Components/BoxComponent.h"
#include "EngineUtils.h"
#include "Gameplay/LRLocomotionComponent.h"
#include "Gameplay/LRMovementRules.h"

/**
 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
 */
ALRNoiseArea::ALRNoiseArea()
{
	PrimaryActorTick.bCanEverTick = false;
	Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
	SetRootComponent(Bounds);
	Bounds->SetCollisionProfileName(TEXT("Trigger"));
	Bounds->OnComponentBeginOverlap.AddDynamic(this, &ALRNoiseArea::HandleBeginOverlap);
	Bounds->OnComponentEndOverlap.AddDynamic(this, &ALRNoiseArea::HandleEndOverlap);
}

/**
 * @brief 处理 Handle Begin Overlap 事件，将引擎回调转换为对应领域状态更新；加入集合后重新求值环境。
 * @param component 参与本次操作的运行时对象 `component`；函数会检查空值和所需接口。
 * @param otherActor 参与本次操作的运行时对象 `otherActor`；函数会检查空值和所需接口。
 * @param otherComponent 参与本次操作的运行时对象 `otherComponent`；函数会检查空值和所需接口。
 * @param otherBodyIndex 本次操作使用的计数、增量或索引 `otherBodyIndex`；由函数校验合法范围。
 * @param bFromSweep 布尔开关 `bFromSweep`；true 表示启用或条件成立，false 表示禁用或条件不成立。
 * @param sweepResult 本次领域操作的结构化数据 `sweepResult`；字段语义由对应 USTRUCT 定义。
 */
void ALRNoiseArea::HandleBeginOverlap(UPrimitiveComponent* component, AActor* otherActor, UPrimitiveComponent* otherComponent,
	const int32 otherBodyIndex, const bool bFromSweep, const FHitResult& sweepResult)
{
	if (!otherActor || !otherActor->FindComponentByClass<ULRLocomotionComponent>())
	{
		return;
	}
	OverlappingActors.AddUnique(otherActor);
	RefreshActorEnvironment(otherActor);
}

/**
 * @brief 处理 Handle End Overlap 事件，将引擎回调转换为对应领域状态更新；离开区域后按剩余重叠集合重新求值环境。
 * @param component 参与本次操作的运行时对象 `component`；函数会检查空值和所需接口。
 * @param otherActor 参与本次操作的运行时对象 `otherActor`；函数会检查空值和所需接口。
 * @param otherComponent 参与本次操作的运行时对象 `otherComponent`；函数会检查空值和所需接口。
 * @param otherBodyIndex 本次操作使用的计数、增量或索引 `otherBodyIndex`；由函数校验合法范围。
 */
void ALRNoiseArea::HandleEndOverlap(UPrimitiveComponent* component, AActor* otherActor, UPrimitiveComponent* otherComponent,
	const int32 otherBodyIndex)
{
	if (!otherActor)
	{
		return;
	}
	OverlappingActors.Remove(otherActor);
	RefreshActorEnvironment(otherActor);
}

/**
 * @brief 按固定优先级（Indoor > OutdoorStealth > Outdoor）从所有覆盖该角色的噪声区域重新解析环境并应用；无区域时默认 Outdoor。
 * @param actor 本次查询、交互或事件涉及的 Actor。
 */
void ALRNoiseArea::RefreshActorEnvironment(AActor* actor)
{
	ULRLocomotionComponent* locomotion = actor ? actor->FindComponentByClass<ULRLocomotionComponent>() : nullptr;
	if (!locomotion || !GetWorld())
	{
		return;
	}
	TArray<ELRNoiseEnvironment> environments;
	for (TActorIterator<ALRNoiseArea> it(GetWorld()); it; ++it)
	{
		if (it->OverlappingActors.Contains(actor))
		{
			environments.Add(it->Environment);
		}
	}
	locomotion->SetNoiseEnvironment(LRMovementRules::ResolveEnvironmentFromSet(environments));
}
