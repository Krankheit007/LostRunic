/**
 * @file LRMenuWidgetController.h
 * @brief 统一菜单控制器：维护背包/笔记/收集品/暂停/存档槽 Tab 状态，绑定 Inventory 与内容定义，构建只读 FLRInventorySnapshot；菜单关闭期间只标记 dirty，打开或可见期间收到领域事件才重建并广播统一 OnSnapshotChanged。
 *
 * 关联文件：LRMenuWidgetController.cpp；所属领域：UI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Data/LRContentRows.h"
#include "UI/LRUITypes.h"
#include "UObject/Object.h"

#include "LRMenuWidgetController.generated.h"

class AActor;
class ULRGameContentSet;
class ULRInventoryComponent;
class ULRUITuning;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLRMenuScreenChanged, ELRScreenType, previousScreen, ELRScreenType, currentScreen);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRSnapshotChanged, const FLRInventorySnapshot&, snapshot);

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Menu Widget Controller"))
class LOSTRUNIC_API ULRMenuWidgetController : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * @brief 绑定库存、内容定义与 UI 调优，并订阅库存领域事件；重复绑定同一库存时不做任何事。
	 * @param inventory 参与本次操作的运行时对象 `inventory`；函数会检查空值和所需接口。
	 * @param contentSet 数据或调优来源 `contentSet`；调用期间只读，并按稳定 ID 解析内容。
	 * @param uiTuning 数据或调优来源 `uiTuning`；调用期间只读，并按稳定 ID 解析内容。
	 */
	void Initialize(ULRInventoryComponent* inventory, ULRGameContentSet* contentSet, ULRUITuning* uiTuning);

	/**
	 * @brief 切换到指定菜单页面并广播变化；同一时刻只保留一个可见菜单。打开/切换时立即重建并广播快照。
	 * @param screen 本次操作使用的 `screen` 枚举或模式值。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool OpenScreen(ELRScreenType screen);
	/**
	 * @brief 关闭当前菜单页面并广播页面变化，焦点由 PlayerController 重新分配。
	 */
	void CloseScreen();

	/**
	 * @brief 构建统一菜单的库存快照；传入 itemUseTarget 时计算每件物品与目标的兼容性（交互选物模式）。
	 *        只读 View Model：Widget 只能消费，所有动作回到 InventoryComponent。
	 * @param inventory 参与本次操作的运行时对象 `inventory`；函数会检查空值和所需接口。
	 * @param itemUseTarget 本次规则检查或操作的目标对象；交互选物模式下传入，普通浏览传 nullptr。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|UI")
	FLRInventorySnapshot BuildInventorySnapshot(const ULRInventoryComponent* inventory, AActor* itemUseTarget = nullptr) const;

	/**
	 * @brief 装备指定武器；合法性由 InventoryComponent 校验，成功后经领域事件驱动快照重建与广播。
	 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Inventory|Weapon")
	bool EquipSelectedWeapon(FName itemId);

	/**
	 * @brief 由阅读表行构建笔记视图（含 Locked 占位），按 ReadingId 字典序；Locked 只暴露稳定 ID 与“？？？”。
	 *        独立静态纯函数：DataTable 迭代只是外壳，该规则可被自动化测试直接覆盖。
	 * @param rows 本次领域操作的结构化数据 `rows`；字段语义由对应 USTRUCT 定义。
	 * @param inventory 参与本次操作的运行时对象 `inventory`；函数会检查空值和所需接口。
	 * @param outViews 输出笔记视图数组；调用前内容被清空。
	 */
	static void BuildNoteViews(const TArray<FLRReadingRow>& rows, const ULRInventoryComponent& inventory, TArray<FLRNoteView>& outViews);

	/**
	 * @brief 查询 Open Screen；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	ELRScreenType GetOpenScreen() const { return OpenScreenType; }

	/**
	 * @brief 查询 Inventory；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|UI")
	ULRInventoryComponent* GetInventory() const { return Inventory; }

	/**
	 * @brief 查询 Cached Snapshot；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|UI")
	const FLRInventorySnapshot& GetCachedSnapshot() const { return CachedSnapshot; }

	/** 当 Menu Screen Changed 发生时广播；蓝图可绑定该委托以更新表现，不应在回调中改写核心规则。  */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|UI")
	FLRMenuScreenChanged OnMenuScreenChanged;

	/** 当快照重建完成时广播（打开菜单或菜单可见期间收到领域事件）；Widget 只消费表现数据。  */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|UI")
	FLRSnapshotChanged OnSnapshotChanged;

private:
	/** 库存条目变化：菜单打开时立即重建并广播，否则只标记 dirty。 */
	UFUNCTION()
	void HandleInventoryChanged(FName itemId, int32 newCount);
	/** 笔记集合变化：菜单打开时立即重建并广播，否则只标记 dirty。 */
	UFUNCTION()
	void HandleNotesChanged();
	/** 收藏品集合变化：菜单打开时立即重建并广播，否则只标记 dirty。 */
	UFUNCTION()
	void HandleCollectiblesChanged();
	/** 武器选择变化：菜单打开时立即重建并广播，否则只标记 dirty。 */
	UFUNCTION()
	void HandleSelectedWeaponChanged();

	/**
	 * @brief 标记快照 dirty；菜单打开或可见期间收到领域事件时立即重建并广播。
	 */
	void MarkSnapshotDirty();
	/**
	 * @brief 用当前库存重建快照并广播 OnSnapshotChanged；清空 dirty 标记。
	 */
	void RebuildSnapshot();

	/** Inventory 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TObjectPtr<ULRInventoryComponent> Inventory;

	/** Content Set 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TObjectPtr<ULRGameContentSet> ContentSet;

	/** UI Tuning 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TObjectPtr<ULRUITuning> UITuning;

	/** Open Screen Type 的内部运行时数据；不参与蓝图配置。 */
	ELRScreenType OpenScreenType = ELRScreenType::None;

	/** Cached Snapshot 的只读 View Model；由领域事件驱动重建。 */
	FLRInventorySnapshot CachedSnapshot;

	/** 菜单关闭期间收到领域事件时置位；打开菜单时立即重建。 */
	bool bSnapshotDirty = false;
};
