// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @file TwinStickPlayerController.h
 * @brief 保留 Unreal TwinStick 模板玩法，用于回归和 PIE 冒烟；它与 /Game/LostRunic 的“家”切片相互独立，不承载 LostRunic 核心叙事规则。
 *
 * 关联文件：TwinStickPlayerController.cpp；所属领域：Variant_TwinStick。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TwinStickPlayerController.generated.h"

class UInputMappingContext;
class ATwinStickCharacter;

/**
 *  Simple Player Controller for a Twin Stick Shooter game
 *  Manages input mapping contexts
 *  Respawns the pawn if it is destroyed
 */
UCLASS(abstract, Config="Game")
class ATwinStickPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Default Mapping Contexts 的领域数据，由所属类型负责维护和校验。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Mobile Excluded Mapping Contexts 的领域数据，由所属类型负责维护和校验。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Mobile Controls Widget Class 的软类或类默认引用，用于创建对应蓝图实例。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Mobile Controls Widget 的领域数据，由所属类型负责维护和校验。  */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Force Touch Controls 的开关；true 表示启用，false 表示禁用。 C++ 安全默认值为 `false`。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Character Class 的软类或类默认引用，用于创建对应蓝图实例。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, Category="Respawn")
	TSubclassOf<ATwinStickCharacter> CharacterClass;

protected:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 在进入世界后解析运行时依赖、绑定事件并启动所需计时器；构造阶段不访问 World 或玩家对象。
	 */
	virtual void BeginPlay() override;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 绑定 PlayerController 使用的 Enhanced Input Action；具体按键仍由 Input Mapping Context 资产决定。
	 */
	virtual void SetupInputComponent() override;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 处理 On Possess 事件，将引擎回调转换为对应领域状态更新。
	 * @param InPawn Controller 新接管的 Pawn；期望为 ALRGuardCharacter。
	 */
	virtual void OnPossess(APawn* InPawn) override;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 处理 On Pawn Destroyed 事件，将引擎回调转换为对应领域状态更新。
	 * @param DestroyedActor 参与本次操作的运行时对象 `DestroyedActor`；函数会检查空值和所需接口。
	 */
	UFUNCTION()
	void OnPawnDestroyed(AActor* DestroyedActor);

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 判断 Should Use Touch Controls 对应条件；不产生玩法副作用。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool ShouldUseTouchControls() const;
};
