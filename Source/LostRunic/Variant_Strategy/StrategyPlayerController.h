// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @file StrategyPlayerController.h
 * @brief 保留 Unreal Strategy 模板玩法，用于回归和 PIE 冒烟；它与 /Game/LostRunic 的“家”切片相互独立，不承载 LostRunic 核心叙事规则。
 *
 * 关联文件：StrategyPlayerController.cpp；所属领域：Variant_Strategy。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "StrategyPlayerController.generated.h"

class AStrategyPawn;
class UInputMappingContext;
class UNiagaraSystem;
struct FInputActionValue;
class AStrategyHUD;
class UInputAction;
struct FInputActionInstance;
class AStrategyUnit;
class UStrategyTouchControls;

/**
 *  Player Controller for a top-down strategy game.
 *  Handles unit selection and commands.
 *  Implements both mouse and touch controls.
 */
UCLASS(abstract)
class AStrategyPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Controlled Camera Pawn 的内部运行时数据；不参与蓝图配置。 */
	TObjectPtr<AStrategyPawn> ControlledCameraPawn;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Strategy HUD 的内部运行时数据；不参与蓝图配置。 */
	TObjectPtr<AStrategyHUD> StrategyHUD;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputMappingContext* MouseMappingContext;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputMappingContext* TouchMappingContext;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Selection Modifier 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	bool bSelectionModifier = false;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Double Tap Active 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	bool bDoubleTapActive = false;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Allow Interaction 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	bool bAllowInteraction = true;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveCameraAction;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ZoomCameraAction;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ResetCameraAction;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* SelectClickAction;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* SelectClickAdditiveAction;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* SelectAllDoubleClickAction;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* SelectHoldAction;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* InteractClickAction;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* InteractHoldAction;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* SelectionModifierAction;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* TouchPrimaryHoldAction;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* TouchSecondaryAction;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Mobile Controls Widget 的内部运行时数据；不参与蓝图配置。 */
	TObjectPtr<UStrategyTouchControls> MobileControlsWidget;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Mobile Controls Widget Class 的软类或类默认引用，用于创建对应蓝图实例。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, Category="Input")
	TSubclassOf<UStrategyTouchControls> MobileControlsWidgetClass;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Force Touch Controls 的开关；true 表示启用，false 表示禁用。 C++ 安全默认值为 `false`。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, Category="Input")
	bool bForceTouchControls = false;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Selection Radius 的空间距离参数，默认使用 Unreal 厘米单位。 C++ 安全默认值为 `250.0f`。 可在对应资产、DataTable 行或蓝图实例中配置。编辑器约束：单位 `cm`，最小值 `0`，最大值 `10000`。 */
	UPROPERTY(EditAnywhere, Category="Input", meta = (ClampMin = 0, ClampMax = 10000, Units = "cm"))
	float SelectionRadius = 250.0f;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Starting Drag Scroll Position 的内部运行时数据；不参与蓝图配置。 */
	FVector2D StartingDragScrollPosition;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Starting Box Selection Position 的内部运行时数据；不参与蓝图配置。 */
	FVector2D StartingBoxSelectionPosition;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Last Touch Drag Scroll Time 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	float LastTouchDragScrollTime = 0.0f;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Touch Drag Scroll Hold Time 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `0.15f`。 可在对应资产、DataTable 行或蓝图实例中配置。编辑器约束：单位 `s`，最小值 `0`，最大值 `1`。 */
	UPROPERTY(EditAnywhere, Category="Input", meta = (ClampMin = 0, ClampMax = 1, Units = "s"))
	float TouchDragScrollHoldTime = 0.15f;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Camera Zoom 的内部运行时数据；不参与蓝图配置。 */
	float CameraZoom = 0.0f;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Default Zoom 的内部运行时数据；不参与蓝图配置。 */
	float DefaultZoom = 1000.0f;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Min Zoom Level 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `1000.0f`。 可在对应资产、DataTable 行或蓝图实例中配置。编辑器约束：最小值 `0`，最大值 `10000`。 */
	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = 0, ClampMax = 10000))
	float MinZoomLevel = 1000.0f;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Max Zoom Level 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `2500.0f`。 可在对应资产、DataTable 行或蓝图实例中配置。编辑器约束：最小值 `0`，最大值 `10000`。 */
	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = 0, ClampMax = 10000))
	float MaxZoomLevel = 2500.0f;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Zoom Scaling 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `100.0f`。 可在对应资产、DataTable 行或蓝图实例中配置。编辑器约束：最小值 `0`，最大值 `1000`。 */
	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = 0, ClampMax = 1000))
	float ZoomScaling = 100.0f;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Drag Multiplier 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `0.1f`。 可在对应资产、DataTable 行或蓝图实例中配置。编辑器约束：最小值 `0`，最大值 `10000`。 */
	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = 0, ClampMax = 10000))
	float DragMultiplier = 0.1f;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Selection Trace Channel 的领域数据，由所属类型负责维护和校验。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, Category = "Selection")
	TEnumAsByte<ETraceTypeQuery> SelectionTraceChannel;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Controlled Units 的内部运行时数据；不参与蓝图配置。 */
	TArray<AStrategyUnit*> ControlledUnits;

public:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 */
	AStrategyPlayerController();

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
	virtual void OnPossess(APawn* InPawn);

public:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Drag Select Units 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 * @param Units 调用方提供的 `Units`，只在本次操作范围内使用。
	 */
	void DragSelectUnits(const TArray<AStrategyUnit*>& Units);

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 查询 Selected Units；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	const TArray<AStrategyUnit*>& GetSelectedUnits();

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 查询 Default Zoom Percentage；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	float GetDefaultZoomPercentage() const;

protected:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 判断 Should Use Touch Controls 对应条件；不产生玩法副作用。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool ShouldUseTouchControls() const;

	// mouse + keyboard input

protected:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 执行 Move Camera 的玩法动作；输入层只提供语义，合法性由对应领域组件决定。
	 * @param Value 本次输入、状态更新或测试使用的值。
	 */
	void MoveCamera(const FInputActionValue& Value);

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Zoom Camera 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 * @param Value 本次输入、状态更新或测试使用的值。
	 */
	void ZoomCamera(const FInputActionValue& Value);

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Reset Camera 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 * @param Value 本次输入、状态更新或测试使用的值。
	 */
	void ResetCamera(const FInputActionValue& Value);

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 执行 Select Hold Started 的纯规则或事务判定，失败时提供结构化原因。
	 * @param Value 本次输入、状态更新或测试使用的值。
	 */
	void SelectHoldStarted(const FInputActionValue& Value);

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 执行 Select Hold Triggered 的纯规则或事务判定，失败时提供结构化原因。
	 * @param Value 本次输入、状态更新或测试使用的值。
	 */
	void SelectHoldTriggered(const FInputActionValue& Value);

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 执行 Select Hold Completed 的纯规则或事务判定，失败时提供结构化原因。
	 * @param Value 本次输入、状态更新或测试使用的值。
	 */
	void SelectHoldCompleted(const FInputActionValue& Value);

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 执行 Select Click 的纯规则或事务判定，失败时提供结构化原因。
	 * @param Value 本次输入、状态更新或测试使用的值。
	 */
	void SelectClick(const FInputActionValue& Value);

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 执行 Select Click Additive 的纯规则或事务判定，失败时提供结构化原因。
	 * @param Value 本次输入、状态更新或测试使用的值。
	 */
	void SelectClickAdditive(const FInputActionValue& Value);

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 执行 Select All Double Click 的纯规则或事务判定，失败时提供结构化原因。
	 * @param Value 本次输入、状态更新或测试使用的值。
	 */
	void SelectAllDoubleClick(const FInputActionValue& Value);

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 执行 Interact Hold Started 的玩法动作；输入层只提供语义，合法性由对应领域组件决定。
	 * @param Value 本次输入、状态更新或测试使用的值。
	 */
	void InteractHoldStarted(const FInputActionValue& Value);

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 执行 Interact Hold Triggered 的玩法动作；输入层只提供语义，合法性由对应领域组件决定。
	 * @param Value 本次输入、状态更新或测试使用的值。
	 */
	void InteractHoldTriggered(const FInputActionValue& Value);

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 执行 Interact Click 的玩法动作；输入层只提供语义，合法性由对应领域组件决定。
	 * @param Value 本次输入、状态更新或测试使用的值。
	 */
	void InteractClick(const FInputActionValue& Value);

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Touch Primary Hold Started 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 * @param Value 本次输入、状态更新或测试使用的值。
	 */
	void TouchPrimaryHoldStarted(const FInputActionValue& Value);

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Touch Primary Hold Triggered 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 * @param Instance 调用方提供的 `Instance`，只在本次操作范围内使用。
	 */
	void TouchPrimaryHoldTriggered(const FInputActionInstance& Instance);

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Touch Primary Hold Completed 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 * @param Value 本次输入、状态更新或测试使用的值。
	 */
	void TouchPrimaryHoldCompleted(const FInputActionValue& Value);

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Touch Secondary Triggered 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 * @param Value 本次输入、状态更新或测试使用的值。
	 */
	void TouchSecondaryTriggered(const FInputActionValue& Value);

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Touch Secondary Completed 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 * @param Value 本次输入、状态更新或测试使用的值。
	 */
	void TouchSecondaryCompleted(const FInputActionValue& Value);

	// commands

public:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Do Select Command 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 * @param SelectLocation 空间值 `SelectLocation`；距离和位置使用 Unreal 厘米单位。
	 * @param bAdditiveSelection 布尔开关 `bAdditiveSelection`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool DoSelectCommand(const FVector& SelectLocation, bool bAdditiveSelection);

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Do Select All Units On Screen Command 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 */
	void DoSelectAllUnitsOnScreenCommand();

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Do Deselect All Units Command 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 */
	void DoDeselectAllUnitsCommand();

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Do Toggle Select All Units Command 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 */
	void DoToggleSelectAllUnitsCommand();

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Do Camera Drag Scroll Command 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 * @param CurrentCursorPosition 调用方提供的 `CurrentCursorPosition`，只在本次操作范围内使用。
	 */
	void DoCameraDragScrollCommand(const FVector2D& CurrentCursorPosition);

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Do Move Units Command 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 * @param GoalLocation 空间值 `GoalLocation`；距离和位置使用 Unreal 厘米单位。
	 */
	void DoMoveUnitsCommand(const FVector& GoalLocation);

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Do Camera Modify Zoom Command 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 * @param ZoomDelta 调用方提供的 `ZoomDelta`，只在本次操作范围内使用。
	 */
	void DoCameraModifyZoomCommand(float ZoomDelta);

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Do Camera Reset Zoom Command 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 */
	void DoCameraResetZoomCommand();

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Do Camera Set Zoom Percentage Command 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 * @param Percentage 调用方提供的 `Percentage`，只在本次操作范围内使用。
	 */
	void DoCameraSetZoomPercentageCommand(float Percentage);

protected:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 查询 Closest Selected Unit To Location；不修改领域状态。
	 * @param TargetLocation 空间值 `TargetLocation`；距离和位置使用 Unreal 厘米单位。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	AStrategyUnit* GetClosestSelectedUnitToLocation(FVector TargetLocation);

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 查询 Mouse Location For Player；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	FVector2D GetMouseLocationForPlayer();

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 查询 Location Under Cursor；不修改领域状态。
	 * @param Location 世界空间位置，Unreal 单位为厘米。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool GetLocationUnderCursor(FVector& Location);

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 查询 Location Under Finger；不修改领域状态。
	 * @param Location 世界空间位置，Unreal 单位为厘米。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool GetLocationUnderFinger(FVector& Location);

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Project Touch Point To World Space 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	FVector ProjectTouchPointToWorldSpace();

protected:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 BP_Cursor Feedback 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 * @param Location 世界空间位置，Unreal 单位为厘米。
	 * @param bPositive 布尔开关 `bPositive`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="Cursor", meta = (DisplayName="Cursor Feedback"))
	void BP_CursorFeedback(FVector Location, bool bPositive);
};
