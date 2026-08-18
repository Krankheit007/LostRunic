/**
 * @file LRInteractionTypes.h
 * @brief 实现统一交互契约：按距离、总朝向角、遮挡和当前状态筛选唯一目标，并以结构化选项和结果连接 UI、背包选物及可交互对象。
 *
 * 关联文件：Interaction 目录内调用该公共契约的实现文件；所属领域：Interaction。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Core/LRTypes.h"
#include "GameplayTagContainer.h"

#include "LRInteractionTypes.generated.h"

class UInputAction;
class USceneComponent;

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Interaction Range"))
enum class ELRInteractionRange : uint8
{
	None UMETA(DisplayName = "None"),
	FarHint UMETA(DisplayName = "Far Hint"),
	Outline UMETA(DisplayName = "Outline"),
	Executable UMETA(DisplayName = "Executable")
};

/** World-facing presentation state computed by the player interaction component. */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Interaction Presentation State"))
enum class ELRInteractionPresentationState : uint8
{
	None UMETA(DisplayName = "None"),
	FarHint UMETA(DisplayName = "Far Hint"),
	NearOutline UMETA(DisplayName = "Near Outline"),
	Focused UMETA(DisplayName = "Focused")
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Interaction Option"))
struct LOSTRUNIC_API FLRInteractionOption
{
	GENERATED_BODY()

	/** Action Tag 的 Gameplay Tag 条件或分类，用于数据驱动规则与诊断。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FGameplayTag ActionTag;

	/** Prompt 的领域数据，由所属类型负责维护和校验。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FText Prompt;

	/** Max Distance Override 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `0.0f`。 可在对应资产、DataTable 行或蓝图实例中配置。编辑器约束：单位 `cm`，最小值 `0.0`。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "0.0", Units = "cm"))
	float MaxDistanceOverride = 0.0f;

	/** Required Mode 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `ELRPerceptionMode::Normal`。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	ELRPerceptionMode RequiredMode = ELRPerceptionMode::Normal;

	/** Required Item Tags 的 Gameplay Tag 条件或分类，用于数据驱动规则与诊断。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FGameplayTagQuery RequiredItemTags;
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Interaction Result"))
struct LOSTRUNIC_API FLRInteractionResult
{
	GENERATED_BODY()

	/** Success 的开关；true 表示启用，false 表示禁用。 C++ 安全默认值为 `false`。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	bool bSuccess = false;

	/** Action Tag 的 Gameplay Tag 条件或分类，用于数据驱动规则与诊断。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	FGameplayTag ActionTag;

	/** Failure Reason 的领域数据，由所属类型负责维护和校验。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	FGameplayTag FailureReason;

};

/** HUD-owned copy of the focused interaction. The weak target never extends actor lifetime. */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Interaction Prompt"))
struct LOSTRUNIC_API FLRInteractionPromptView
{
	GENERATED_BODY()

	/** Weak world target used for diagnostics; the HUD never extends actor lifetime. */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	TWeakObjectPtr<AActor> Target;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	FText Prompt;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	FGameplayTag ActionTag;

	/** Semantic Enhanced Input action used by the widget to resolve the current device icon. */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<UInputAction> InputAction;

	/** Scene component followed by the HUD for world-space prompt projection. */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction|Presentation")
	TWeakObjectPtr<USceneComponent> PromptAnchor;

	/** World-space offset applied to the resolved prompt anchor, normally a Z lift. */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction|Presentation")
	FVector PromptWorldOffset = FVector::ZeroVector;

	/** Device-specific display text resolved by the HUD controller from the active mappings. */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction|Presentation")
	FText InputKeyText;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	bool bVisible = false;

	/** Compares all controller-relevant prompt fields, including same-target presentation changes. */
	bool HasSamePresentationAs(const FLRInteractionPromptView& other) const
	{
		return Target == other.Target
			&& Prompt.EqualTo(other.Prompt)
			&& ActionTag == other.ActionTag
			&& InputAction == other.InputAction
			&& PromptAnchor == other.PromptAnchor
			&& PromptWorldOffset.Equals(other.PromptWorldOffset)
			&& InputKeyText.EqualTo(other.InputKeyText)
			&& bVisible == other.bVisible;
	}
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
struct LOSTRUNIC_API FLRInteractionCandidateScore
{
	/** Distance 的内部运行时数据；不参与蓝图配置。 */
	float DistanceSquared = 0.0f;
	/** Forward Dot 的内部运行时数据；不参与蓝图配置。 */
	float ForwardDot = 1.0f;
	/** Occluded 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	bool bOccluded = false;
	/** Mode Allowed 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	bool bModeAllowed = true;
	/** Items Allowed 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	bool bItemsAllowed = true;
};
