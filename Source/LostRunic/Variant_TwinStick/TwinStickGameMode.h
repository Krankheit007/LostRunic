// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @file TwinStickGameMode.h
 * @brief 保留 Unreal TwinStick 模板玩法，用于回归和 PIE 冒烟；它与 /Game/LostRunic 的“家”切片相互独立，不承载 LostRunic 核心叙事规则。
 *
 * 关联文件：TwinStickGameMode.cpp；所属领域：Variant_TwinStick。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TwinStickGameMode.generated.h"

class UTwinStickUI;

/**
 *  Simple Game Mode for a Twin Stick Shooter game.
 *  Manages the score and UI
 */
UCLASS(abstract)
class ATwinStickGameMode : public AGameModeBase
{
	GENERATED_BODY()

protected:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** UIWidget Class 的软类或类默认引用，用于创建对应蓝图实例。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, Category="Twin Stick")
	TSubclassOf<UTwinStickUI> UIWidgetClass;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** UIWidget 的内部运行时数据；不参与蓝图配置。 */
	TObjectPtr<UTwinStickUI> UIWidget;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Score 的内部运行时数据；不参与蓝图配置。 */
	int32 Score = 0;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Combo 的内部运行时数据；不参与蓝图配置。 */
	int32 Combo = 1;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Combo Increment 的内部运行时数据；不参与蓝图配置。 */
	int32 ComboIncrement = 0;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Combo Increment Max 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `5`。 可在对应资产、DataTable 行或蓝图实例中配置。编辑器约束：最小值 `0`，最大值 `10`。 */
	UPROPERTY(EditAnywhere, Category="Twin Stick", meta=(ClampMin = 0, ClampMax = 10))
	int32 ComboIncrementMax = 5;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Combo Cap 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `4`。 可在对应资产、DataTable 行或蓝图实例中配置。编辑器约束：最小值 `0`，最大值 `10`。 */
	UPROPERTY(EditAnywhere, Category="Twin Stick", meta=(ClampMin = 0, ClampMax = 10))
	int32 ComboCap = 4;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Combo Cooldown 的时间参数，单位为秒；由所属调优或资产提供权威值。 C++ 安全默认值为 `3.0f`。 可在对应资产、DataTable 行或蓝图实例中配置。编辑器约束：单位 `s`，最小值 `0`，最大值 `10`。 */
	UPROPERTY(EditAnywhere, Category="Twin Stick", meta=(ClampMin = 0, ClampMax = 10, Units = "s"))
	float ComboCooldown = 3.0f;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Last Combo Time 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	float LastComboTime = 0.0f;

	/** Combo Timer 的运行时句柄，用于取消回调并避免 Tick；不在蓝图中配置。 */
	FTimerHandle ComboTimer;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** NPCCap 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `20`。 可在对应资产、DataTable 行或蓝图实例中配置。编辑器约束：最小值 `0`，最大值 `100`。 */
	UPROPERTY(EditAnywhere, Category="Twin Stick", meta=(ClampMin = 0, ClampMax = 100))
	int32 NPCCap = 20;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** NPCCount 的内部运行时数据；不参与蓝图配置。 */
	int32 NPCCount = 0;

public:

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

public:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Item Used 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 * @param Value 本次输入、状态更新或测试使用的值。
	 */
	void ItemUsed(int32 Value);

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Score Update 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 * @param Value 本次输入、状态更新或测试使用的值。
	 */
	void ScoreUpdate(int32 Value);

protected:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 根据当前领域状态构建 Create UI 所需的数据，不把临时对象作为长期存档标识。
	 */
	void CreateUI();

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Combo Update 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 */
	void ComboUpdate();

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Reset Combo Cooldown 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 */
	void ResetComboCooldown();

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Reset Combo 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 */
	void ResetCombo();

public:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 判断 Can Spawn NPCs 对应条件；不产生玩法副作用。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool CanSpawnNPCs();

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Increase NPCs 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 */
	void IncreaseNPCs();

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Decrease NPCs 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 */
	void DecreaseNPCs();
};
