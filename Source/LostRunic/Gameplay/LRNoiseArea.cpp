/**
 * @file LRNoiseArea.cpp
 * @brief 实现角色移动模式、按移动距离产生脚步和室内外噪声区域等基础玩法能力；数值来自调优资产，不使用无理由 Tick。
 *
 * 关联文件：LRNoiseArea.h；所属领域：Gameplay。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Gameplay/LRNoiseArea.h"

#include "Components/BoxComponent.h"
#include "Gameplay/LRLocomotionComponent.h"

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
}

/**
 * @brief 处理 Handle Begin Overlap 事件，将引擎回调转换为对应领域状态更新。
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
	if (ULRLocomotionComponent* locomotion = otherActor ? otherActor->FindComponentByClass<ULRLocomotionComponent>() : nullptr)
	{
		locomotion->SetNoiseEnvironment(Environment);
	}
}
