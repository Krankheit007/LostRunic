// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @file TwinStickSpawner.h
 * @brief 保留 Unreal TwinStick 模板玩法，用于回归和 PIE 冒烟；它与 /Game/LostRunic 的“家”切片相互独立，不承载 LostRunic 核心叙事规则。
 *
 * 关联文件：TwinStickSpawner.cpp；所属领域：Variant_TwinStick。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TwinStickNPC.h"
#include "TwinStickSpawner.generated.h"

class ARecastNavMesh;

/**
 *  A simple NPC spawner for a Twin Stick Shooter game
 */
UCLASS(abstract)
class ATwinStickSpawner : public AActor
{
	GENERATED_BODY()

protected:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** NPCClass 的软类或类默认引用，用于创建对应蓝图实例。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, Category="NPC Spawner")
	TSubclassOf<ATwinStickNPC> NPCClass;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Spawn Group Delay 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `5.0f`。 可在对应资产、DataTable 行或蓝图实例中配置。编辑器约束：单位 `s`，最小值 `0`，最大值 `20`。 */
	UPROPERTY(EditAnywhere, Category="NPC Spawner", meta = (ClampMin = 0, ClampMax = 20, Units = "s"))
	float SpawnGroupDelay = 5.0f;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Min Spawn Delay 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `0.33f`。 可在对应资产、DataTable 行或蓝图实例中配置。编辑器约束：单位 `s`，最小值 `0`，最大值 `2`。 */
	UPROPERTY(EditAnywhere, Category="NPC Spawner", meta = (ClampMin = 0, ClampMax = 2, Units = "s"))
	float MinSpawnDelay = 0.33f;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Max Spawn Delay 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `0.66f`。 可在对应资产、DataTable 行或蓝图实例中配置。编辑器约束：单位 `s`，最小值 `0`，最大值 `2`。 */
	UPROPERTY(EditAnywhere, Category="NPC Spawner", meta = (ClampMin = 0, ClampMax = 2, Units = "s"))
	float MaxSpawnDelay = 0.66f;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Spawn Radius 的空间距离参数，默认使用 Unreal 厘米单位。 C++ 安全默认值为 `600.0f`。 可在对应资产、DataTable 行或蓝图实例中配置。编辑器约束：单位 `cm`，最小值 `0`，最大值 `20`。 */
	UPROPERTY(EditAnywhere, Category="NPC Spawner", meta = (ClampMin = 0, ClampMax = 20, Units = "cm"))
	float SpawnRadius = 600.0f;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Spawn Group Size 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `3`。 可在对应资产、DataTable 行或蓝图实例中配置。编辑器约束：最小值 `0`，最大值 `10`。 */
	UPROPERTY(EditAnywhere, Category="NPC Spawner", meta = (ClampMin = 0, ClampMax = 10))
	int32 SpawnGroupSize = 3;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Spawn Count 的内部运行时数据；不参与蓝图配置。 */
	int32 SpawnCount = 0;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Spawn Group Timer 的运行时句柄，用于取消回调并避免 Tick；不在蓝图中配置。 */
	FTimerHandle SpawnGroupTimer;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Spawn NPCTimer 的运行时句柄，用于取消回调并避免 Tick；不在蓝图中配置。 */
	FTimerHandle SpawnNPCTimer;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Nav Data 的内部运行时数据；不参与蓝图配置。 */
	TObjectPtr<ARecastNavMesh> NavData;

public:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 */
	ATwinStickSpawner();

protected:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 在进入世界后解析运行时依赖、绑定事件并启动所需计时器；构造阶段不访问 World 或玩家对象。
	 */
	virtual void BeginPlay() override;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 解除委托并清理计时器或缓存，避免关卡切换和对象销毁后继续收到回调。
	 * @param EndPlayReason Unreal 提供的结束原因，用于区分销毁、关卡切换和退出。
	 */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

protected:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Spawn NPCGroup 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 */
	void SpawnNPCGroup();

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Spawn NPC 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 */
	void SpawnNPC();

};
