/**
 * @file LRRoomVolume.h
 * @brief 定义室内奔跑噪声的房间传播体积：维护重叠守卫集合与相邻房间拓扑，支持旋转体积的局部空间包含判定；房间传播只广播表现事件，不调用 ReportNoiseEvent（防双计）。
 *
 * 关联文件：LRRoomVolume.cpp；所属领域：Gameplay。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "GameFramework/Actor.h"

#include "LRRoomVolume.generated.h"

class UBoxComponent;
class ULRAlertComponent;

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Room Volume"))
class LOSTRUNIC_API ALRRoomVolume : public AActor
{
	GENERATED_BODY()

public:
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 */
	ALRRoomVolume();

	/**
	 * @brief 查询所有覆盖指定位置（支持旋转体积，BoxComponent 局部空间 + extent 判定）的房间体积。
	 * @param world 本次查询所在的 World。
	 * @param location 世界空间位置，Unreal 单位为厘米。
	 * @param outRooms 输出匹配的房间体积集合。
	 */
	static void FindRoomsAtLocation(const UWorld* world, const FVector location, TArray<ALRRoomVolume*>& outRooms);

	/**
	 * @brief 查询 Overlapping Guards；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	const TArray<TWeakObjectPtr<AActor>>& GetOverlappingGuards() const { return OverlappingGuards; }

	/**
	 * @brief 查询 Adjacent Rooms；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	const TArray<TWeakObjectPtr<ALRRoomVolume>>& GetAdjacentRooms() const { return AdjacentRooms; }

private:
	/**
	 * @brief 处理 Handle Begin Overlap 事件：带警戒组件的守卫加入房间集合。
	 * @param component 参与本次操作的运行时对象 `component`；函数会检查空值和所需接口。
	 * @param otherActor 参与本次操作的运行时对象 `otherActor`；函数会检查空值和所需接口。
	 * @param otherComponent 参与本次操作的运行时对象 `otherComponent`；函数会检查空值和所需接口。
	 * @param otherBodyIndex 本次操作使用的计数、增量或索引 `otherBodyIndex`；由函数校验合法范围。
	 * @param bFromSweep 布尔开关 `bFromSweep`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 * @param sweepResult 本次领域操作的结构化数据 `sweepResult`；字段语义由对应 USTRUCT 定义。
	 */
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* component, AActor* otherActor, UPrimitiveComponent* otherComponent,
		int32 otherBodyIndex, bool bFromSweep, const FHitResult& sweepResult);

	/**
	 * @brief 处理 Handle End Overlap 事件：守卫离开房间集合。
	 * @param component 参与本次操作的运行时对象 `component`；函数会检查空值和所需接口。
	 * @param otherActor 参与本次操作的运行时对象 `otherActor`；函数会检查空值和所需接口。
	 * @param otherComponent 参与本次操作的运行时对象 `otherComponent`；函数会检查空值和所需接口。
	 * @param otherBodyIndex 本次操作使用的计数、增量或索引 `otherBodyIndex`；由函数校验合法范围。
	 */
	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* component, AActor* otherActor, UPrimitiveComponent* otherComponent,
		int32 otherBodyIndex);

	/** Bounds 的开关；true 表示启用，false 表示禁用。 仅在蓝图或详情面板中查看，不可编辑。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Room", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> Bounds;

	/** Room Id 的稳定 FName/GUID 标识；用于定义查询和存档，不依赖显示名或临时 Actor 名称。 C++ 安全默认值为 `NAME_None`。 可在关卡中的蓝图实例详情面板配置。 */
	UPROPERTY(EditInstanceOnly, Category = "Room")
	FName RoomId = NAME_None;

	/** Adjacent Rooms 的领域数据，由所属类型负责维护和校验。 可在关卡中的蓝图实例详情面板配置。 */
	UPROPERTY(EditInstanceOnly, Category = "Room")
	TArray<TWeakObjectPtr<ALRRoomVolume>> AdjacentRooms;

	/** Overlapping Guards 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	TArray<TWeakObjectPtr<AActor>> OverlappingGuards;
};
