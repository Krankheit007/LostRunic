/**
 * @file LRSaveTypes.h
 * @brief 声明可恢复锚点、库存/叙事分块、槽位类型、写入类型、请求结果与 Memory 事务阶段；磁盘字段使用稳定 FName ID。
 *
 * 关联文件：LRSaveTypes.cpp；所属领域：Save。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Core/LRTypes.h"

#include "LRSaveTypes.generated.h"

class ULRSaveGame;

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Resume Anchor"))
struct LOSTRUNIC_API FLRResumeAnchor
{
	GENERATED_BODY()

	/** Map Id 的稳定 FName/GUID 标识；用于定义查询和存档，不依赖显示名或临时 Actor 名称。 C++ 安全默认值为 `NAME_None`。 可在对应资产、DataTable 行或蓝图实例中配置。该字段参与存档序列化。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Resume")
	FName MapId = NAME_None;

	/** Anchor Id 的稳定 FName/GUID 标识；用于定义查询和存档，不依赖显示名或临时 Actor 名称。 C++ 安全默认值为 `NAME_None`。 可在对应资产、DataTable 行或蓝图实例中配置。该字段参与存档序列化。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Resume")
	FName AnchorId = NAME_None;

	/** Location 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `FVector::ZeroVector`。 可在对应资产、DataTable 行或蓝图实例中配置。该字段参与存档序列化。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Resume")
	FVector Location = FVector::ZeroVector;

	/** Rotation 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `FRotator::ZeroRotator`。 可在对应资产、DataTable 行或蓝图实例中配置。该字段参与存档序列化。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Resume")
	FRotator Rotation = FRotator::ZeroRotator;

	/**
	 * @brief 判断 Is Valid 对应条件；不产生玩法副作用。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool IsValid() const { return !MapId.IsNone() && !AnchorId.IsNone(); }
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Save Inventory"))
struct LOSTRUNIC_API FLRSaveInventoryChunk
{
	GENERATED_BODY()

	/** 按稳定物品 ID 保存的持有数量。 蓝图可读取但不可写入。该字段参与存档序列化。 */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Inventory")
	TMap<FName, int32> ItemCounts;

	/** 四个快捷栏保存的物品稳定 ID，空槽使用 NAME_None。 蓝图可读取但不可写入。该字段参与存档序列化。 */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Inventory")
	TArray<FName> QuickSlots;

	/** Selected Quick Slot 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `0`。 蓝图可读取但不可写入。该字段参与存档序列化。 */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Inventory")
	int32 SelectedQuickSlot = 0;

	/** 已阅读笔记的稳定 ID 集合。 蓝图可读取但不可写入。该字段参与存档序列化。 */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Inventory")
	TSet<FName> NoteIds;

	/** 已取得收藏品的稳定 ID 集合。 蓝图可读取但不可写入。该字段参与存档序列化。 */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Inventory")
	TSet<FName> CollectibleIds;
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Save Narrative"))
struct LOSTRUNIC_API FLRSaveNarrativeChunk
{
	GENERATED_BODY()

	/** Completed Event Ids 的领域数据，由所属类型负责维护和校验。 蓝图可读取但不可写入。该字段参与存档序列化。 */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Narrative")
	TSet<FName> CompletedEventIds;

	/** Memory Event Ids 的领域数据，由所属类型负责维护和校验。 蓝图可读取但不可写入。该字段参与存档序列化。 */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Narrative")
	TSet<FName> MemoryEventIds;

	/** 累计死亡次数，关键存档 A 后必须持久化。 C++ 安全默认值为 `0`。 蓝图可读取但不可写入。该字段参与存档序列化。 */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Narrative")
	int32 DeathCount = 0;
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Save Slot Type"))
enum class ELRSaveSlotType : uint8
{
	Auto UMETA(DisplayName = "Auto"),
	Manual UMETA(DisplayName = "Manual")
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Save Write Kind"))
enum class ELRSaveWriteKind : uint8
{
	Auto UMETA(DisplayName = "Auto"),
	Manual UMETA(DisplayName = "Manual"),
	Critical UMETA(DisplayName = "Critical"),
	MemoryEntry UMETA(DisplayName = "Memory Entry"),
	MemoryEvent UMETA(DisplayName = "Memory Event"),
	MemoryReturn UMETA(DisplayName = "Memory Return")
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Save Request Result"))
enum class ELRSaveRequestResult : uint8
{
	Queued UMETA(DisplayName = "Queued"),
	Scheduled UMETA(DisplayName = "Scheduled"),
	Loaded UMETA(DisplayName = "Loaded"),
	RejectedInvalidSlot UMETA(DisplayName = "Rejected Invalid Slot"),
	RejectedMemoryManual UMETA(DisplayName = "Rejected In Memory"),
	RejectedInvalidAnchor UMETA(DisplayName = "Rejected Invalid Anchor"),
	RejectedUnavailableMap UMETA(DisplayName = "Rejected Unavailable Map"),
	MissingOrCorrupt UMETA(DisplayName = "Missing Or Corrupt")
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Memory Transaction Phase"))
enum class ELRMemoryTransactionPhase : uint8
{
	None UMETA(DisplayName = "None"),
	AwaitingMemoryWorld UMETA(DisplayName = "Awaiting Memory World"),
	SavingEntry UMETA(DisplayName = "Saving Entry"),
	InMemory UMETA(DisplayName = "In Memory"),
	AwaitingResumeWorld UMETA(DisplayName = "Awaiting Resume World"),
	SavingReturn UMETA(DisplayName = "Saving Return")
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
USTRUCT(meta = (DisplayName = "Lost Runic Queued Save Request"))
struct LOSTRUNIC_API FLRQueuedSaveRequest
{
	GENERATED_BODY()

	/** Snapshot 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TObjectPtr<ULRSaveGame> Snapshot;

	/** Slot Name 的内部运行时数据；不参与蓝图配置。 */
	FString SlotName;
	/** Reason Id 的内部运行时数据；不参与蓝图配置。 */
	FName ReasonId = NAME_None;
	/** Kind 的内部运行时数据；不参与蓝图配置。 */
	ELRSaveWriteKind Kind = ELRSaveWriteKind::Auto;
	/** Retry Attempt 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	int32 RetryAttempt = 0;
};

namespace LRSaveIds
{
	LOSTRUNIC_API extern const FName AutoSlotReason;
	LOSTRUNIC_API extern const FName MemoryEntryReason;
	LOSTRUNIC_API extern const FName MemoryReturnReason;
	LOSTRUNIC_API extern const FName MemoryMapId;
}
