/**
 * @file LRNarrativeTypes.h
 * @brief 实现 SUDS 对话、Reading DataTable、条件分支和一次性剧情事件；稳定 FName ID 进入存档，显示全文与推进下一句的二段确认由控制层维护。
 *
 * 关联文件：Narrative 目录内调用该公共契约的实现文件；所属领域：Narrative。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Core/LRTypes.h"
#include "GameplayTagContainer.h"

#include "LRNarrativeTypes.generated.h"

class UTexture2D;

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Narrative Session Type"))
enum class ELRNarrativeSessionType : uint8
{
	None UMETA(DisplayName = "None"),
	Dialogue UMETA(DisplayName = "Dialogue"),
	Reading UMETA(DisplayName = "Reading")
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Narrative Action"))
enum class ELRNarrativeAction : uint8
{
	Rejected UMETA(DisplayName = "Rejected"),
	Started UMETA(DisplayName = "Started"),
	RevealCurrentText UMETA(DisplayName = "Reveal Current Text"),
	Advanced UMETA(DisplayName = "Advanced"),
	AwaitChoice UMETA(DisplayName = "Await Choice"),
	Completed UMETA(DisplayName = "Completed")
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Narrative Choice"))
struct LOSTRUNIC_API FLRNarrativeChoice
{
	GENERATED_BODY()

	/** Choice Id 的稳定 FName/GUID 标识；用于定义查询和存档，不依赖显示名或临时 Actor 名称。 C++ 安全默认值为 `NAME_None`。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative")
	FName ChoiceId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative")
	int32 ChoiceIndex = INDEX_NONE;

	/** Text 的领域数据，由所属类型负责维护和校验。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative")
	FText Text;

	/** Next Content Id 的稳定 FName/GUID 标识；用于定义查询和存档，不依赖显示名或临时 Actor 名称。 C++ 安全默认值为 `NAME_None`。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative")
	FName NextContentId = NAME_None;
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Narrative Page"))
struct LOSTRUNIC_API FLRNarrativePage
{
	GENERATED_BODY()

	/** Session Type 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `ELRNarrativeSessionType::None`。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative")
	ELRNarrativeSessionType SessionType = ELRNarrativeSessionType::None;

	/** Content Id 的稳定 FName/GUID 标识；用于定义查询和存档，不依赖显示名或临时 Actor 名称。 C++ 安全默认值为 `NAME_None`。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative")
	FName ContentId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative")
	FName ScriptId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative")
	FString TextId;

	/** Speaker Id 的稳定 FName/GUID 标识；用于定义查询和存档，不依赖显示名或临时 Actor 名称。 C++ 安全默认值为 `NAME_None`。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative")
	FName SpeakerId = NAME_None;

	/** Title 的领域数据，由所属类型负责维护和校验。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative")
	FText Title;

	/** Text 的领域数据，由所属类型负责维护和校验。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative")
	FText Text;

	/** Dialogue line text; Reading continues to use Text as its body field. */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Dialogue")
	FText LineText;

	/** Localized speaker display name supplied by SUDS. */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Dialogue")
	FText SpeakerName;

	/** Portrait 的领域数据，由所属类型负责维护和校验。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative")
	TObjectPtr<UTexture2D> Portrait = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Dialogue")
	bool bShowSpeakerName = false;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Dialogue")
	bool bShowPortrait = false;

	/** Choices 的领域数据，由所属类型负责维护和校验。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative")
	TArray<FLRNarrativeChoice> Choices;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative")
	bool bSimpleContinue = true;
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Narrative Result"))
struct LOSTRUNIC_API FLRNarrativeResult
{
	GENERATED_BODY()

	/** Success 的开关；true 表示启用，false 表示禁用。 C++ 安全默认值为 `false`。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative")
	bool bSuccess = false;

	/** Action Enhanced Input Action 资产；C++ 绑定其语义，具体键位在 Mapping Context 中配置。 C++ 安全默认值为 `ELRNarrativeAction::Rejected`。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative")
	ELRNarrativeAction Action = ELRNarrativeAction::Rejected;

	/** Content Id 的稳定 FName/GUID 标识；用于定义查询和存档，不依赖显示名或临时 Actor 名称。 C++ 安全默认值为 `NAME_None`。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative")
	FName ContentId = NAME_None;

	/** Failure Reason 的领域数据，由所属类型负责维护和校验。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative")
	FGameplayTag FailureReason;
};
