// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @file LostRunicPlayerController.h
 * @brief 提供 Unreal 模块入口及原始 TopDown 模板兼容类；新的“家”垂直切片使用 Framework、State、Interaction、AI、Narrative 与 Save 目录中的 LR 领域实现。
 *
 * 关联文件：LostRunicPlayerController.cpp；所属领域：Root。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */

#pragma once

#include "CoreMinimal.h"
//#include "Templates/SubclassOf.h"
#include "GameFramework/PlayerController.h"
#include "LostRunicPlayerController.generated.h"

class UNiagaraSystem;
class UInputMappingContext;
class UInputAction;
class UPathFollowingComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  Player controller for a top-down perspective game.
 *  Implements point and click based controls
 */
UCLASS(abstract)
class ALostRunicPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Path Following Component 的领域数据，由所属类型负责维护和校验。 仅在蓝图或详情面板中查看，不可编辑。 */
	UPROPERTY(VisibleDefaultsOnly, Category = AI)
	TObjectPtr<UPathFollowingComponent> PathFollowingComponent;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Short Press Threshold 的领域数据，由所属类型负责维护和校验。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, Category="Input")
	float ShortPressThreshold;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** FXCursor 的领域数据，由所属类型负责维护和校验。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UNiagaraSystem> FXCursor;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Default Mapping Context Enhanced Input Mapping Context 资产，用于对应输入模式。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Set Destination Click Action Enhanced Input Action 资产；C++ 绑定其语义，具体键位在 Mapping Context 中配置。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> SetDestinationClickAction;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Set Destination Touch Action Enhanced Input Action 资产；C++ 绑定其语义，具体键位在 Mapping Context 中配置。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> SetDestinationTouchAction;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	uint32 bMoveToMouseCursor : 1;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	uint32 bIsTouch : 1;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Cached Destination 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	FVector CachedDestination;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Follow Time 的内部运行时数据；不参与蓝图配置。 */
	float FollowTime = 0.0f;

public:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 */
	ALostRunicPlayerController();

protected:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 绑定 PlayerController 使用的 Enhanced Input Action；具体按键仍由 Input Mapping Context 资产决定。
	 */
	virtual void SetupInputComponent() override;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 处理 On Input Started 事件，将引擎回调转换为对应领域状态更新。
	 */
	void OnInputStarted();
	/**
	 * @brief 处理 On Set Destination Triggered 事件，将引擎回调转换为对应领域状态更新。
	 */
	void OnSetDestinationTriggered();
	/**
	 * @brief 处理 On Set Destination Released 事件，将引擎回调转换为对应领域状态更新。
	 */
	void OnSetDestinationReleased();
	/**
	 * @brief 处理 On Touch Triggered 事件，将引擎回调转换为对应领域状态更新。
	 */
	void OnTouchTriggered();
	/**
	 * @brief 处理 On Touch Released 事件，将引擎回调转换为对应领域状态更新。
	 */
	void OnTouchReleased();

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 根据最新领域状态刷新 Update Cached Destination，并仅在值变化时通知订阅者。
	 */
	void UpdateCachedDestination();
};


