/**
 * @file LRHUDWidgetController.h
 * @brief 实现 HUD、状态遮罩、对话/阅读、背包/笔记/收藏、暂停、存档槽和过场的控制器边界。UI 订阅领域事件并负责表现，不参与核心规则判定。
 *
 * 关联文件：LRHUDWidgetController.cpp；所属领域：UI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Core/LRTypes.h"
#include "UObject/Object.h"

#include "LRHUDWidgetController.generated.h"

class ALRCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLRHUDPerceptionModeChanged, ELRPerceptionMode, mode, FGameplayTag, reason);

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic HUD Widget Controller"))
class LOSTRUNIC_API ULRHUDWidgetController : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * @brief 更新 Observed Character，并在需要时同步组件状态或广播变化事件。
	 * @param character 参与本次操作的运行时对象 `character`；函数会检查空值和所需接口。
	 */
	void SetObservedCharacter(ALRCharacter* character);
	/**
	 * @brief 释放子系统事件绑定和运行时缓存。
	 */
	void Deinitialize();

	/**
	 * @brief 查询 Current Mode；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	ELRPerceptionMode GetCurrentMode() const { return CurrentMode; }

	/** 当 Perception Mode Changed 发生时广播；蓝图可绑定该委托以更新表现，不应在回调中改写核心规则。  */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|UI")
	FLRHUDPerceptionModeChanged OnPerceptionModeChanged;

private:
	/**
	 * @brief 处理 Handle State Changed 事件，将引擎回调转换为对应领域状态更新。
	 * @param currentMode 本次操作使用的 `currentMode` 枚举或模式值。
	 * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
	 */
	UFUNCTION()
	void HandleStateChanged(ELRPerceptionMode currentMode, FGameplayTag reason);

	/** Observed Character 的内部运行时数据；不参与蓝图配置。 */
	TWeakObjectPtr<ALRCharacter> ObservedCharacter;
	/** 当前已提交的心理状态；仅状态组件可修改，蓝图只能读取。该字段由 C++ 在运行时维护，不在蓝图中配置。 */
	ELRPerceptionMode CurrentMode = ELRPerceptionMode::Normal;
};
