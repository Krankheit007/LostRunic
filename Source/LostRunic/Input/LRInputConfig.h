/**
 * @file LRInputConfig.h
 * @brief 定义 Enhanced Input 的语义资产集合和 Gameplay、Dialogue、Menu、Transition 上下文，具体键鼠与手柄按键由输入资产配置，C++ 只绑定动作语义。
 *
 * 关联文件：LRInputConfig.cpp；所属领域：Input。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "LRInputConfig.generated.h"

class UInputAction;
class UInputMappingContext;

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Input Config"))
class LOSTRUNIC_API ULRInputConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Gameplay Context Enhanced Input Mapping Context 资产，用于对应输入模式。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Contexts")
	TObjectPtr<UInputMappingContext> GameplayContext;

	/** Dialogue Context Enhanced Input Mapping Context 资产，用于对应输入模式。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Contexts")
	TObjectPtr<UInputMappingContext> DialogueContext;

	/** Menu Context Enhanced Input Mapping Context 资产，用于对应输入模式。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Contexts")
	TObjectPtr<UInputMappingContext> MenuContext;

	/** Transition Context Enhanced Input Mapping Context 资产，用于对应输入模式。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Contexts")
	TObjectPtr<UInputMappingContext> TransitionContext;

	/** Move Action Enhanced Input Action 资产；C++ 绑定其语义，具体键位在 Mapping Context 中配置。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Movement")
	TObjectPtr<UInputAction> MoveAction;

	/** Sneak Action Enhanced Input Action 资产；C++ 绑定其语义，具体键位在 Mapping Context 中配置。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Movement")
	TObjectPtr<UInputAction> SneakAction;

	/** Run Action Enhanced Input Action 资产；C++ 绑定其语义，具体键位在 Mapping Context 中配置。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Movement")
	TObjectPtr<UInputAction> RunAction;

	/** Interact Action Enhanced Input Action 资产；C++ 绑定其语义，具体键位在 Mapping Context 中配置。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Gameplay")
	TObjectPtr<UInputAction> InteractAction;

	/** Close Eyes Action Enhanced Input Action 资产；C++ 绑定其语义，具体键位在 Mapping Context 中配置。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|State")
	TObjectPtr<UInputAction> CloseEyesAction;

	/** Open Eyes Action Enhanced Input Action 资产；C++ 绑定其语义，具体键位在 Mapping Context 中配置。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|State")
	TObjectPtr<UInputAction> OpenEyesAction;

	/** Confirm Action Enhanced Input Action 资产；C++ 绑定其语义，具体键位在 Mapping Context 中配置。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|UI")
	TObjectPtr<UInputAction> ConfirmAction;

	/** Cancel Action Enhanced Input Action 资产；C++ 绑定其语义，具体键位在 Mapping Context 中配置。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|UI")
	TObjectPtr<UInputAction> CancelAction;

	/** Attack Action Enhanced Input Action 资产；C++ 绑定其语义，具体键位在 Mapping Context 中配置（沿用原 UseQuickSlot 键位）。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Gameplay")
	TObjectPtr<UInputAction> AttackAction;

	/** 已废弃：旧 UseQuickSlot 输入引用，仅作为资产迁移回退保留，运行时不存在快捷栏语义。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Gameplay|Deprecated", meta = (DeprecatedProperty, DisplayName = "Use Quick Slot Action (Deprecated)"))
	TObjectPtr<UInputAction> UseQuickSlotAction;

	/** Toggle Crouch Action Enhanced Input Action 资产；C++ 绑定其语义，具体键位在 Mapping Context 中配置。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Gameplay")
	TObjectPtr<UInputAction> ToggleCrouchAction;

	/** Open Journal Action Enhanced Input Action 资产；C++ 绑定其语义，具体键位在 Mapping Context 中配置。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|UI")
	TObjectPtr<UInputAction> OpenJournalAction;

	/** Pause Action Enhanced Input Action 资产；C++ 绑定其语义，具体键位在 Mapping Context 中配置。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|UI")
	TObjectPtr<UInputAction> PauseAction;

	/**
	 * @brief 校验当前资产的必填引用、数值边界及跨字段关系，并输出可诊断错误。
	 * @param outError 输出校验失败原因；成功时保持为空。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool Validate(FString& outError) const;

#if WITH_EDITOR
	/**
	 * @brief 接入 Unreal Data Validation，将领域校验错误报告给编辑器。
	 * @param context 用于本次条件匹配的 `context` 标签或上下文。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif
};
