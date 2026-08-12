/**
 * @file LRItemUseTypes.h
 * @brief 实现背包、笔记、收藏品和统一物品使用事务；Interaction 与 Attack 两个入口共用解析事务，目标效果成功后按 bConsumable 消费，失败不触碰库存。
 *
 * 关联文件：Items 目录内调用该公共契约的实现文件；所属领域：Items。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Core/LRTypes.h"
#include "GameplayTagContainer.h"

#include "LRItemUseTypes.generated.h"

class AActor;

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Item Use Entry Point"))
enum class ELRItemUseEntryPoint : uint8
{
	Interaction UMETA(DisplayName = "Interaction"),
	Attack UMETA(DisplayName = "Attack")
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Item Use Request"))
struct LOSTRUNIC_API FLRItemUseRequest
{
	GENERATED_BODY()

	/** Item Id 的稳定 FName/GUID 标识；用于定义查询和存档，不依赖显示名或临时 Actor 名称。 C++ 安全默认值为 `NAME_None`（空手攻击时合法）。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Item Use")
	FName ItemId = NAME_None;

	/** Target 的领域数据，由所属类型负责维护和校验。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Item Use")
	TObjectPtr<UObject> Target;

	/** Instigator 的领域数据，由所属类型负责维护和校验。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Item Use")
	TObjectPtr<AActor> Instigator;

	/** Entry Point 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `ELRItemUseEntryPoint::Interaction`。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Item Use")
	ELRItemUseEntryPoint EntryPoint = ELRItemUseEntryPoint::Interaction;

	/** 当前已提交的心理状态；仅状态组件可修改，蓝图只能读取。 C++ 安全默认值为 `ELRPerceptionMode::Normal`。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Item Use")
	ELRPerceptionMode CurrentMode = ELRPerceptionMode::Normal;

	/** Action Tag 的 Gameplay Tag 条件或分类，用于数据驱动规则与诊断。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Item Use")
	FGameplayTag ActionTag;
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Item Use Result"))
struct LOSTRUNIC_API FLRItemUseResult
{
	GENERATED_BODY()

	/** Success 的开关；true 表示启用，false 表示禁用。 C++ 安全默认值为 `false`。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Item Use")
	bool bSuccess = false;

	/** Consumed 的开关；true 表示启用，false 表示禁用。 C++ 安全默认值为 `false`。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Item Use")
	bool bConsumed = false;

	/** Item Id 的稳定 FName/GUID 标识；用于定义查询和存档，不依赖显示名或临时 Actor 名称。 C++ 安全默认值为 `NAME_None`。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Item Use")
	FName ItemId = NAME_None;

	/** Event Id 的稳定 FName/GUID 标识；用于定义查询和存档，不依赖显示名或临时 Actor 名称。 C++ 安全默认值为 `NAME_None`。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Item Use")
	FName EventId = NAME_None;

	/** Failure Reason 的领域数据，由所属类型负责维护和校验。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Item Use")
	FGameplayTag FailureReason;
};
