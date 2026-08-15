/**
 * @file LRInventoryComponent.h
 * @brief 按稳定物品 ID 保存堆叠条目（数量 + 单调获得顺序）、武器选择、笔记与收藏品 ID；只维护物品状态和武器选择，不理解攻击条件或使用入口。
 *
 * 关联文件：LRInventoryComponent.cpp；所属领域：Items。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Save/LRSaveTypes.h"
#include "Save/LRSaveV2Types.h"

#include "LRInventoryComponent.generated.h"

class ULRCollectibleDefinition;
class ULRGameContentSet;
class ULRItemDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLRInventoryChanged, FName, itemId, int32, newCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLRInventoryNotesChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLRInventoryCollectiblesChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLRInventorySelectedWeaponChanged);

/** 统一菜单的共享命名容量：Bag 8 / Note 12 / Collectible 12。容量是开发期契约，不是正常截断策略；ContentSet 编辑器校验与快照越界检查同时兜底。 */
namespace LRMenuCapacity
{
	constexpr int32 Bag = 8;
	constexpr int32 Notes = 12;
	constexpr int32 Collectibles = 12;
}

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Add Item Result"))
enum class ELRAddItemResult : uint8
{
	Success UMETA(DisplayName = "Success"),
	InventoryFull UMETA(DisplayName = "Inventory Full"),
	InvalidDefinition UMETA(DisplayName = "Invalid Definition"),
	InvalidQuantity UMETA(DisplayName = "Invalid Quantity")
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Add Collectible Result"))
enum class ELRAddCollectibleResult : uint8
{
	Success UMETA(DisplayName = "Success"),
	AlreadyOwned UMETA(DisplayName = "Already Owned"),
	InvalidDefinition UMETA(DisplayName = "Invalid Definition"),
	AtCapacity UMETA(DisplayName = "At Capacity")
};

/** 按稳定物品 ID 保存的堆叠条目；Quantity 同时表示持有数量和剩余可使用次数。 */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Inventory Entry"))
struct LOSTRUNIC_API FLRInventoryEntry
{
	GENERATED_BODY()

	/** Item Id 的稳定 FName/GUID 标识；用于定义查询和存档，不依赖显示名或临时 Actor 名称。 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FName ItemId = NAME_None;

	/** 持有数量；一次性物品同时表示剩余可使用次数，无限物品固定为 1。 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 Quantity = 0;

	/** 首次从 0 增加到 1 时分配的单调获得顺序；堆叠增加保留原顺序，完全移除后重新获得分配新顺序。 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int64 AcquisitionSequence = 0;
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(ClassGroup = "Lost Runic", BlueprintType, meta = (BlueprintSpawnableComponent, DisplayName = "Lost Runic Inventory"))
class LOSTRUNIC_API ULRInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 */
	ULRInventoryComponent();

	/**
	 * @brief 在进入世界后解析运行时依赖、绑定事件并启动所需计时器；构造阶段不访问 World 或玩家对象。
	 */
	virtual void BeginPlay() override;

	/**
	 * @brief 按稳定物品 ID 建立定义索引，供库存查询、标签汇总和物品使用。
	 * @param definitions 数据或调优来源 `definitions`；调用期间只读，并按稳定 ID 解析内容。
	 */
	void InitializeDefinitions(const TArray<ULRItemDefinition*>& definitions);

	/**
	 * @brief 按稳定收藏品 ID 建立定义索引；未注册的收藏品 ID 会被 AddCollectibleId 拒绝为 InvalidDefinition。
	 * @param collectibles 数据或调优来源 `collectibles`；调用期间只读，并按稳定 ID 解析内容。
	 */
	void InitializeCollectibleDefinitions(const TArray<ULRCollectibleDefinition*>& collectibles);

	/**
	 * @brief 按稳定物品 ID 增加库存数量；达到 MaxStackSize 时返回 InventoryFull 且不改变库存。
	 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
	 * @param count 本次操作使用的计数、增量或索引 `count`；由函数校验合法范围。
	 * @return Success、InventoryFull、InvalidDefinition 或 InvalidQuantity 的结构化结果。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Inventory")
	ELRAddItemResult AddItem(FName itemId, int32 count = 1);

	/**
	 * @brief 查询 Item Count；不修改领域状态。
	 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Inventory")
	int32 GetItemCount(FName itemId) const;

	/**
	 * @brief 判断 Has Item 对应条件；不产生玩法副作用。
	 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
	 * @param count 本次操作使用的计数、增量或索引 `count`；由函数校验合法范围。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Inventory")
	bool HasItem(FName itemId, int32 count = 1) const;

	/**
	 * @brief 查询当前库存堆叠条目；调用方只读，不依赖 TMap 迭代顺序判断“最早获得”。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Inventory")
	const TMap<FName, FLRInventoryEntry>& GetEntries() const { return Entries; }

	/**
	 * @brief 查询 Item Count；不修改领域状态。
	 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	const FLRInventoryEntry* FindEntry(FName itemId) const;

	/**
	 * @brief 把玩家明确选择的武器设置为指定已持有且带 Item.Category.Weapon 的物品；None 是合法状态。
	 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Inventory|Weapon")
	bool SetSelectedWeapon(FName itemId);

	/**
	 * @brief 查询 Selected Weapon；未选择或选择已失效时返回 None。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Inventory|Weapon")
	FName GetSelectedWeapon() const;

	/**
	 * @brief 查询攻击实际使用的武器：显式选择仍有效时返回它，否则按 AcquisitionSequence 返回最早获得的现存武器，没有武器时返回 None。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Inventory|Weapon")
	FName GetEffectiveWeapon() const;

	/**
	 * @brief 把稳定笔记 ID 加入已读集合；集合语义保证重复阅读不会产生重复记录。
	 * @param noteId 稳定标识 `noteId`；用于内容查询和存档，不依赖显示名或数组序号。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Inventory|Journal")
	bool AddNoteId(FName noteId);

	/**
	 * @brief 把稳定收藏品 ID 加入已收集集合；重复添加返回 AlreadyOwned 且不修改世界状态。
	 * @param collectibleId 稳定标识 `collectibleId`；用于内容查询和存档，不依赖显示名或数组序号。
	 * @return Success、AlreadyOwned 或 InvalidDefinition 的结构化结果。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Inventory|Journal")
	ELRAddCollectibleResult AddCollectibleId(FName collectibleId);

	/**
	 * @brief 查询 Owned Item Tags；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Inventory")
	FGameplayTagContainer GetOwnedItemTags() const;

	/**
	 * @brief 查询 Owned Item Ids；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Inventory")
	TArray<FName> GetOwnedItemIds() const;

	/**
	 * @brief 按获得顺序（AcquisitionSequence）再按 ItemId 返回持有条目，供统一菜单快照固定排序。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	TArray<FLRInventoryEntry> GetOwnedEntriesOrdered() const;

	/**
	 * @brief 查询 Note Ids；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Inventory|Journal")
	TArray<FName> GetNoteIds() const;

	/**
	 * @brief 查询 Collectible Ids；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Inventory|Journal")
	TArray<FName> GetCollectibleIds() const;

	/**
	 * @brief 按稳定 ID 或运行时条件查找 Definition，未找到时返回明确失败值。
	 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	ULRItemDefinition* FindDefinition(FName itemId) const;

	/**
	 * @brief 把运行时库存、笔记或收藏品状态复制到存档分块；不填充已废弃的快捷栏字段。
	 * @param outInventory 参与本次操作的运行时对象 `outInventory`；函数会检查空值和所需接口。
	 */
	void CaptureSaveState(FLRSaveInventoryChunk& outInventory) const;
	/**
	 * @brief 从存档分块恢复运行时状态；完全忽略已废弃的快捷栏字段。
	 * @param savedInventory 参与本次操作的运行时对象 `savedInventory`；函数会检查空值和所需接口。
	 */
	void RestoreSaveState(const FLRSaveInventoryChunk& savedInventory);

	void CaptureInventorySaveState(FLRSaveInventoryChunkV2& outInventory) const;
	void RestoreInventorySaveState(const FLRSaveInventoryChunkV2& savedInventory);
	void CaptureNotebookSaveState(FLRSaveNotebookChunk& outNotebook) const;
	void RestoreNotebookSaveState(const FLRSaveNotebookChunk& savedNotebook);
	void CaptureCollectibleSaveState(FLRSaveCollectibleChunk& outCollectible) const;
	void RestoreCollectibleSaveState(const FLRSaveCollectibleChunk& savedCollectible);

	/** 当 Inventory Changed 发生时广播；蓝图可绑定该委托以更新表现，不应在回调中改写核心规则。  */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Inventory")
	FLRInventoryChanged OnInventoryChanged;

	/** 当已读笔记集合变化时广播（增加、存档恢复）。  */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Inventory|Journal")
	FLRInventoryNotesChanged OnNotesChanged;

	/** 当已收集收藏品集合变化时广播（增加、存档恢复）。  */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Inventory|Journal")
	FLRInventoryCollectiblesChanged OnCollectiblesChanged;

	/** 当显式武器选择变化时广播（装备、清空、移除、存档恢复）。  */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Inventory|Weapon")
	FLRInventorySelectedWeaponChanged OnSelectedWeaponChanged;

private:
	/** 数量为 0 时删除条目，并在被删除条目是当前显式武器时清空 SelectedWeaponItemId。 */
	friend class ULRItemUseResolver;

	/**
	 * @brief 在事务成功后扣除一个一次性物品；扣为 0 时删除条目并清理显式武器选择。
	 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool TryConsumeItem(FName itemId);

	/**
	 * @brief 从库存删除条目；若该条目是当前显式武器则清空选择。
	 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
	 */
	void RemoveEntry(FName itemId);

	/**
	 * @brief 判断 Is Weapon 对应条件；不产生玩法副作用。
	 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool IsWeapon(FName itemId) const;

	/** Definitions 定义资产集合；运行时按各资产稳定 ID 建立索引。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<ULRItemDefinition>> Definitions;

	/** Collectible Definitions 定义资产集合；运行时按各资产稳定 ID 建立索引。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<ULRCollectibleDefinition>> CollectibleDefinitions;

	/** 按稳定物品 ID 保存的堆叠条目；数量为 0 时删除条目。 仅在蓝图或详情面板中查看，不可编辑。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
	TMap<FName, FLRInventoryEntry> Entries;

	/** 已阅读笔记的稳定 ID 集合。 仅在蓝图或详情面板中查看，不可编辑。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
	TSet<FName> NoteIds;

	/** 已取得收藏品的稳定 ID 集合。 仅在蓝图或详情面板中查看，不可编辑。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
	TSet<FName> CollectibleIds;

	/** 玩家明确选择的武器稳定 ID；None 是合法状态，回退只在 GetEffectiveWeapon 时惰性计算。 */
	FName SelectedWeaponItemId = NAME_None;

	/** 下一条获得的物品将使用的单调递增获得顺序；不进入存档。 */
	int64 NextAcquisitionSequence = 1;
};
