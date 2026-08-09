// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @file TwinStickProjectile.h
 * @brief 保留 Unreal TwinStick 模板玩法，用于回归和 PIE 冒烟；它与 /Game/LostRunic 的“家”切片相互独立，不承载 LostRunic 核心叙事规则。
 *
 * 关联文件：TwinStickProjectile.cpp；所属领域：Variant_TwinStick。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TwinStickProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;

/**
 *  A simple bouncing projectile for a Twin Stick shooter game
 */
UCLASS(abstract)
class ATwinStickProjectile : public AActor
{
	GENERATED_BODY()

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USphereComponent* CollisionSphere;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Mesh;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UProjectileMovementComponent* ProjectileMovement;

public:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 */
	ATwinStickProjectile();

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

protected:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 处理 On Projectile Stop 事件，将引擎回调转换为对应领域状态更新。
	 * @param ImpactResult 本次领域操作的结构化数据 `ImpactResult`；字段语义由对应 USTRUCT 定义。
	 */
	UFUNCTION()
	void OnProjectileStop(const FHitResult& ImpactResult);

};
