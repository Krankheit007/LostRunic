// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @file TwinStickNPC.h
 * @brief 保留 Unreal TwinStick 模板玩法，用于回归和 PIE 冒烟；它与 /Game/LostRunic 的“家”切片相互独立，不承载 LostRunic 核心叙事规则。
 *
 * 关联文件：TwinStickNPC.cpp；所属领域：Variant_TwinStick。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TwinStickNPC.generated.h"

class ATwinStickPickup;
class ATwinStickNPCDestruction;

/**
 *  A simple enemy NPC for a Twin Stick Shooter game
 *  It's driven by an AI Controller running a behavior tree
 *  Awards points and randomly spawns pickups on death
 */
UCLASS(abstract)
class ATwinStickNPC : public ACharacter
{
	GENERATED_BODY()

protected:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Score 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `1`。 可在对应资产、DataTable 行或蓝图实例中配置。编辑器约束：最小值 `0`，最大值 `100`。 */
	UPROPERTY(EditAnywhere, Category="Score", meta=(ClampMin = 0, ClampMax = 100))
	int32 Score = 1;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Pickup Spawn Chance 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `10`。 可在对应资产、DataTable 行或蓝图实例中配置。编辑器约束：最小值 `0`，最大值 `100`。 */
	UPROPERTY(EditAnywhere, Category="Pickup", meta=(ClampMin = 0, ClampMax = 100))
	int32 PickupSpawnChance = 10;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Pickup Class 的软类或类默认引用，用于创建对应蓝图实例。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, Category="Pickup")
	TSubclassOf<ATwinStickPickup> PickupClass;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Destruction Proxy Class 的软类或类默认引用，用于创建对应蓝图实例。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, Category="Destruction")
	TSubclassOf<ATwinStickNPCDestruction> DestructionProxyClass;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Deferred Destruction Time 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `0.1f`。 可在对应资产、DataTable 行或蓝图实例中配置。编辑器约束：单位 `s`，最小值 `0`，最大值 `5`。 */
	UPROPERTY(EditAnywhere, Category="Pickup", meta=(ClampMin = 0, ClampMax = 5, Units = "s"))
	float DeferredDestructionTime = 0.1f;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Destruction Timer 的运行时句柄，用于取消回调并避免 Tick；不在蓝图中配置。 */
	FTimerHandle DestructionTimer;

public:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Hit 的开关；true 表示启用，false 表示禁用。 C++ 安全默认值为 `false`。 仅在蓝图或详情面板中查看，不可编辑。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="NPC")
	bool bHit = false;

public:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 */
	ATwinStickNPC();

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

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Destroyed 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 */
	virtual void Destroyed() override;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
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
	virtual void NotifyHit(class UPrimitiveComponent* MyComp, AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;

public:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Projectile Impact 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 * @param ForwardVector 调用方提供的 `ForwardVector`，只在本次操作范围内使用。
	 */
	void ProjectileImpact(const FVector& ForwardVector);

protected:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Deferred Destroy 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 */
	void DeferredDestroy();
};
