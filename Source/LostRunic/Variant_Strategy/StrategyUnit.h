// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @file StrategyUnit.h
 * @brief 保留 Unreal Strategy 模板玩法，用于回归和 PIE 冒烟；它与 /Game/LostRunic 的“家”切片相互独立，不承载 LostRunic 核心叙事规则。
 *
 * 关联文件：StrategyUnit.cpp；所属领域：Variant_Strategy。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AIController.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "StrategyUnit.generated.h"

class USphereComponent;
class UEnvQuery;
class UEnvQueryInstanceBlueprintWrapper;

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnitMoveCompletedDelegate, AStrategyUnit*, Unit);

/**
 *  A simple strategy game unit
 *  Rather than react to inputs, it's controlled indirectly by the Strategy Player Controller
 */
UCLASS(abstract)
class AStrategyUnit : public ACharacter
{
	GENERATED_BODY()

private:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USphereComponent* InteractionRange;

protected:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** AIController 的内部运行时数据；不参与蓝图配置。 */
	TObjectPtr<AAIController> AIController;

public:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 */
	AStrategyUnit();

protected:

	/**
	 * @brief 实现 Notify Controller Changed 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 */
	virtual void NotifyControllerChanged() override;

public:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 结束或取消 Stop Moving 流程，并清理本次操作拥有的临时状态。
	 */
	void StopMoving();

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Unit Selected 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 */
	void UnitSelected();

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Unit Deselected 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 */
	void UnitDeselected();

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 执行 Interact 的玩法动作；输入层只提供语义，合法性由对应领域组件决定。
	 * @param Interactor 参与本次操作的运行时对象 `Interactor`；函数会检查空值和所需接口。
	 */
	void Interact(AStrategyUnit* Interactor);

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 执行 Move To Location 的玩法动作；输入层只提供语义，合法性由对应领域组件决定。
	 * @param Location 世界空间位置，Unreal 单位为厘米。
	 * @param bInteract 布尔开关 `bInteract`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 * @param IgnoreList 调用方提供的 `IgnoreList`，只在本次操作范围内使用。
	 */
	void MoveToLocation(const FVector& Location, bool bInteract, const TArray<AStrategyUnit*> IgnoreList);

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 查询 Movement Goal；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	FVector GetMovementGoal() const;

protected:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 处理 On EQSFinished 事件，将引擎回调转换为对应领域状态更新。
	 * @param QueryInstance 调用方提供的 `QueryInstance`，只在本次操作范围内使用。
	 * @param QueryStatus 调用方提供的 `QueryStatus`，只在本次操作范围内使用。
	 */
	UFUNCTION()
	void OnEQSFinished(UEnvQueryInstanceBlueprintWrapper* QueryInstance, EEnvQueryStatus::Type QueryStatus);

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 处理 On Move Finished 事件，将引擎回调转换为对应领域状态更新。
	 * @param RequestID 稳定标识 `RequestID`；用于内容查询和存档，不依赖显示名或数组序号。
	 * @param Result 本次领域操作的结构化数据 `Result`；字段语义由对应 USTRUCT 定义。
	 */
	void OnMoveFinished(FAIRequestID RequestID, const FPathFollowingResult& Result);

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 处理 Handle Move Finished 事件，将引擎回调转换为对应领域状态更新。
	 */
	void HandleMoveFinished();

protected:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 BP_Unit Selected 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="NPC", meta = (DisplayName="Unit Selected"))
	void BP_UnitSelected();

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 BP_Unit Deselected 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="NPC", meta = (DisplayName="Unit Deselected"))
	void BP_UnitDeselected();

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 BP_Stop Animation 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category="NPC", meta = (DisplayName="Stop Animation"))
	void BP_StopAnimation();

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 BP_Interaction Behavior 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 * @param Interactor 参与本次操作的运行时对象 `Interactor`；函数会检查空值和所需接口。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="NPC", meta = (DisplayName="Interaction Behavior"))
	void BP_InteractionBehavior(AStrategyUnit* Interactor);

protected:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Interaction Query 的领域数据，由所属类型负责维护和校验。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, Category="NPC")
	TObjectPtr<UEnvQuery> InteractionQuery;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** No Interaction Query 的领域数据，由所属类型负责维护和校验。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, Category="NPC")
	TObjectPtr<UEnvQuery> NoInteractionQuery;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Movement Acceptance Radius 的空间距离参数，默认使用 Unreal 厘米单位。 C++ 安全默认值为 `100.0f`。 可在对应资产、DataTable 行或蓝图实例中配置。编辑器约束：单位 `cm`，最小值 `0`，最大值 `10000`。 */
	UPROPERTY(EditAnywhere, Category="NPC", meta = (ClampMin = 0, ClampMax = 10000, Units = "cm"))
	float MovementAcceptanceRadius = 100.0f;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Interaction Radius 的空间距离参数，默认使用 Unreal 厘米单位。 C++ 安全默认值为 `250.0f`。 可在对应资产、DataTable 行或蓝图实例中配置。编辑器约束：单位 `cm`，最小值 `0`，最大值 `10000`。 */
	UPROPERTY(EditAnywhere, Category="Input", meta = (ClampMin = 0, ClampMax = 10000, Units = "cm"))
	float InteractionRadius = 250.0f;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Env Query Instance 的内部运行时数据；不参与蓝图配置。 */
	TObjectPtr<UEnvQueryInstanceBlueprintWrapper> EnvQueryInstance;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Current Movement Goal 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	FVector CurrentMovementGoal;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Interact On Arrival 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	bool bInteractOnArrival = false;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Interact Ignore List 的内部运行时数据；不参与蓝图配置。 */
	TArray<AStrategyUnit*> InteractIgnoreList;

public:

	/** On Move Completed 的内部运行时数据；不参与蓝图配置。 */
	FOnUnitMoveCompletedDelegate OnMoveCompleted;
};
