/**
 * @file LRStateTypes.h
 * @brief 声明四种心理状态、状态请求类型以及 Changing/Changed/Rejected 公共请求结果，作为输入、剧情、死亡与表现系统之间的稳定契约。
 *
 * 关联文件：State 目录内调用该公共契约的实现文件；所属领域：State。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Core/LRTypes.h"
#include "GameplayTagContainer.h"

#include "LRStateTypes.generated.h"

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic State Request Type"))
enum class ELRStateRequestType : uint8
{
	None UMETA(DisplayName = "None"),
	CloseEyes UMETA(DisplayName = "Close Eyes"),
	OpenEyes UMETA(DisplayName = "Open Eyes"),
	Death UMETA(DisplayName = "Death"),
	Narrative UMETA(DisplayName = "Narrative")
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic State Change Request"))
struct LOSTRUNIC_API FLRStateChangeRequest
{
	GENERATED_BODY()

	/** Target Mode 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `ELRPerceptionMode::Normal`。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	ELRPerceptionMode TargetMode = ELRPerceptionMode::Normal;

	/** Request Type 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `ELRStateRequestType::None`。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	ELRStateRequestType RequestType = ELRStateRequestType::None;

	/** Source 的领域数据，由所属类型负责维护和校验。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	FGameplayTag Source;
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic State Change Result"))
struct LOSTRUNIC_API FLRStateChangeResult
{
	GENERATED_BODY()

	/** Accepted 的开关；true 表示启用，false 表示禁用。 C++ 安全默认值为 `false`。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bAccepted = false;

	/** Previous Mode 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `ELRPerceptionMode::Normal`。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	ELRPerceptionMode PreviousMode = ELRPerceptionMode::Normal;

	/** 当前已提交的心理状态；仅状态组件可修改，蓝图只能读取。 C++ 安全默认值为 `ELRPerceptionMode::Normal`。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	ELRPerceptionMode CurrentMode = ELRPerceptionMode::Normal;

	/** Reason 的领域数据，由所属类型负责维护和校验。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	FGameplayTag Reason;
};
