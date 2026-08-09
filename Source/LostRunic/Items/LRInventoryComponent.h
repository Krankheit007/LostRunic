/**
 * @file LRInventoryComponent.h
 * @brief 保存物品数量、4 格快捷栏、笔记及收藏品稳定 ID，并把快捷栏使用和背包选物统一提交给 LRItemUseResolver。
 *
 * 关联文件：LRInventoryComponent.cpp；所属领域：Items。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Items/LRItemUseTypes.h"
#include "Save/LRSaveTypes.h"

#include "LRInventoryComponent.generated.h"

class ULRGameContentSet;
class ULRItemDefinition;
class ULRItemUseResolver;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLRInventoryChanged, FName, itemId, int32, newCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLRQuickSlotChanged, int32, slotIndex, FName, itemId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRItemUseResolved, FLRItemUseResult, result);

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
	 * @brief 按稳定物品 ID 增加库存数量；拒绝未知定义、非正数量和溢出结果。
	 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
	 * @param count 本次操作使用的计数、增量或索引 `count`；由函数校验合法范围。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Inventory")
	bool AddItem(FName itemId, int32 count = 1);

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
	 * @brief 把已持有且允许快捷使用的物品 ID 分配到 0-3 快捷栏位。
	 * @param slotIndex 槽位下标；快捷栏为 0-3，手动存档槽按调优上限校验。
	 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Inventory|Quick Slots")
	bool AssignQuickSlot(int32 slotIndex, FName itemId);

	/**
	 * @brief 执行 Select Quick Slot 的纯规则或事务判定，失败时提供结构化原因。
	 * @param slotIndex 槽位下标；快捷栏为 0-3，手动存档槽按调优上限校验。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Inventory|Quick Slots")
	void SelectQuickSlot(int32 slotIndex);

	/**
	 * @brief 执行 Select Adjacent Quick Slot 的纯规则或事务判定，失败时提供结构化原因。
	 * @param direction 本次操作使用的计数、增量或索引 `direction`；由函数校验合法范围。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Inventory|Quick Slots")
	void SelectAdjacentQuickSlot(int32 direction);

	/**
	 * @brief 查询 Quick Slot Item；不修改领域状态。
	 * @param slotIndex 槽位下标；快捷栏为 0-3，手动存档槽按调优上限校验。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Inventory|Quick Slots")
	FName GetQuickSlotItem(int32 slotIndex) const;

	/**
	 * @brief 查询 Selected Quick Slot；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Inventory|Quick Slots")
	int32 GetSelectedQuickSlot() const { return SelectedQuickSlot; }

	/**
	 * @brief 从指定快捷栏取得物品并通过统一物品事务作用于当前目标。
	 * @param slotIndex 槽位下标；快捷栏为 0-3，手动存档槽按调优上限校验。
	 * @param target 本次规则检查或操作的目标对象。
	 * @param currentMode 本次操作使用的 `currentMode` 枚举或模式值。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Inventory|Use")
	FLRItemUseResult UseQuickSlot(int32 slotIndex, AActor* target, ELRPerceptionMode currentMode);

	/**
	 * @brief 执行 Use Item From Selector 的玩法动作；输入层只提供语义，合法性由对应领域组件决定。
	 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
	 * @param target 本次规则检查或操作的目标对象。
	 * @param currentMode 本次操作使用的 `currentMode` 枚举或模式值。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Inventory|Use")
	FLRItemUseResult UseItemFromSelector(FName itemId, AActor* target, ELRPerceptionMode currentMode);

	FLRItemUseRequest BuildUseRequest(FName itemId, int32 sourceSlot, UObject* target,
		ELRPerceptionMode currentMode, ELRItemUseEntryPoint entryPoint) const;
	/**
	 * @brief 执行 Resolve Use Request At Time 的纯规则或事务判定，失败时提供结构化原因。
	 * @param request 不可变领域请求，包含本次操作所需的稳定 ID、来源、目标或原因。
	 * @param currentTimeSeconds 时间值 `currentTimeSeconds`，单位为秒。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	FLRItemUseResult ResolveUseRequestAtTime(const FLRItemUseRequest& request, double currentTimeSeconds);

	/**
	 * @brief 把稳定笔记 ID 加入已读集合；集合语义保证重复阅读不会产生重复记录。
	 * @param noteId 稳定标识 `noteId`；用于内容查询和存档，不依赖显示名或数组序号。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Inventory|Journal")
	bool AddNoteId(FName noteId);

	/**
	 * @brief 把稳定收藏品 ID 加入已收集集合；集合语义保证一次性拾取。
	 * @param collectibleId 稳定标识 `collectibleId`；用于内容查询和存档，不依赖显示名或数组序号。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Inventory|Journal")
	bool AddCollectibleId(FName collectibleId);

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
	 * @brief 把运行时库存、笔记、收藏品或剧情状态复制到存档分块。
	 * @param outInventory 参与本次操作的运行时对象 `outInventory`；函数会检查空值和所需接口。
	 */
	void CaptureSaveState(FLRSaveInventoryChunk& outInventory) const;
	/**
	 * @brief 从存档分块恢复运行时状态，并通过领域 API 保持不变量。
	 * @param savedInventory 参与本次操作的运行时对象 `savedInventory`；函数会检查空值和所需接口。
	 */
	void RestoreSaveState(const FLRSaveInventoryChunk& savedInventory);

	/** 当 Inventory Changed 发生时广播；蓝图可绑定该委托以更新表现，不应在回调中改写核心规则。  */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Inventory")
	FLRInventoryChanged OnInventoryChanged;

	/** 当 Quick Slot Changed 发生时广播；蓝图可绑定该委托以更新表现，不应在回调中改写核心规则。  */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Inventory")
	FLRQuickSlotChanged OnQuickSlotChanged;

	/** 当 Item Use Resolved 发生时广播；蓝图可绑定该委托以更新表现，不应在回调中改写核心规则。  */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Inventory|Use")
	FLRItemUseResolved OnItemUseResolved;

private:
	/** class 的内部运行时数据；不参与蓝图配置。 */
	friend class ULRItemUseResolver;

	/**
	 * @brief 执行 Use Item 的玩法动作；输入层只提供语义，合法性由对应领域组件决定。
	 * @param request 不可变领域请求，包含本次操作所需的稳定 ID、来源、目标或原因。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	FLRItemUseResult UseItem(const FLRItemUseRequest& request);
	/**
	 * @brief 在事务执行前尝试扣除一个物品；目标执行失败时由调用方恢复。
	 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool TryConsumeItem(FName itemId);
	/**
	 * @brief 把 Restore Item 数据应用到运行时对象，并显式处理缺失依赖。
	 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
	 */
	void RestoreItem(FName itemId);

	/** Definitions 定义资产集合；运行时按各资产稳定 ID 建立索引。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<ULRItemDefinition>> Definitions;

	/** 按稳定物品 ID 保存的持有数量。 仅在蓝图或详情面板中查看，不可编辑。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
	TMap<FName, int32> ItemCounts;

	/** 四个快捷栏保存的物品稳定 ID，空槽使用 NAME_None。 仅在蓝图或详情面板中查看，不可编辑。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
	TArray<FName> QuickSlots;

	/** 已阅读笔记的稳定 ID 集合。 仅在蓝图或详情面板中查看，不可编辑。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
	TSet<FName> NoteIds;

	/** 已取得收藏品的稳定 ID 集合。 仅在蓝图或详情面板中查看，不可编辑。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
	TSet<FName> CollectibleIds;

	/** Resolver 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TObjectPtr<ULRItemUseResolver> Resolver;

	/** Selected Quick Slot 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	int32 SelectedQuickSlot = 0;
};
