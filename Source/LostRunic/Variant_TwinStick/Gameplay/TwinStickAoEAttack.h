// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @file TwinStickAoEAttack.h
 * @brief 保留 Unreal TwinStick 模板玩法，用于回归和 PIE 冒烟；它与 /Game/LostRunic 的“家”切片相互独立，不承载 LostRunic 核心叙事规则。
 *
 * 关联文件：TwinStickAoEAttack.cpp；所属领域：Variant_TwinStick。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TwinStickAoEAttack.generated.h"

class UStaticMeshComponent;
class USphereComponent;

/**
 *  A simple persistent AoE attack.
 *  Damages characters that enter for as long as it's active
 */
UCLASS(abstract)
class ATwinStickAoEAttack : public AActor
{
	GENERATED_BODY()

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* SphereVisual;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USphereComponent* CollisionSphere;

protected:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Start Ao ETimer 的运行时句柄，用于取消回调并避免 Tick；不在蓝图中配置。 */
	FTimerHandle StartAoETimer;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Stop Ao ETimer 的运行时句柄，用于取消回调并避免 Tick；不在蓝图中配置。 */
	FTimerHandle StopAoETimer;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Start Ao ETime 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `0.033f`。 可在对应资产、DataTable 行或蓝图实例中配置。编辑器约束：单位 `s`，最小值 `0`，最大值 `5`。 */
	UPROPERTY(EditAnywhere, Category="AoE Attack", meta=(ClampMin = 0, ClampMax = 5, Units = "s"))
	float StartAoETime = 0.033f;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Stop Ao ETime 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `0.5f`。 可在对应资产、DataTable 行或蓝图实例中配置。编辑器约束：单位 `s`，最小值 `0`，最大值 `5`。 */
	UPROPERTY(EditAnywhere, Category="AoE Attack", meta=(ClampMin = 0, ClampMax = 5, Units = "s"))
	float StopAoETime = 0.5f;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Is Ao EActive 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	bool bIsAoEActive = false;

public:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 */
	ATwinStickAoEAttack();

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
	 * @brief 开始 Start Ao E 流程，建立本次操作拥有的状态、委托或计时器。
	 */
	void StartAoE();

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 结束或取消 Stop Ao E 流程，并清理本次操作拥有的临时状态。
	 */
	void StopAoE();

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 BP_Ao EFinished 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="AoE Attack")
	void BP_AoEFinished();

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 处理 On Ao EOverlap 事件，将引擎回调转换为对应领域状态更新。
	 * @param OverlappedComponent 参与本次操作的运行时对象 `OverlappedComponent`；函数会检查空值和所需接口。
	 * @param OtherActor 参与本次操作的运行时对象 `OtherActor`；函数会检查空值和所需接口。
	 * @param OtherComp 调用方提供的 `OtherComp`，只在本次操作范围内使用。
	 * @param OtherBodyIndex 本次操作使用的计数、增量或索引 `OtherBodyIndex`；由函数校验合法范围。
	 * @param bFromSweep 布尔开关 `bFromSweep`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 * @param SweepResult 本次领域操作的结构化数据 `SweepResult`；字段语义由对应 USTRUCT 定义。
	 */
	UFUNCTION()
	void OnAoEOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
