/**
 * @file LRMenuWidgetController.h
 * @brief 实现 HUD、状态遮罩、对话/阅读、背包/笔记/收藏、暂停、存档槽和过场的控制器边界。UI 订阅领域事件并负责表现，不参与核心规则判定。
 *
 * 关联文件：LRMenuWidgetController.cpp；所属领域：UI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "UI/LRUITypes.h"
#include "UObject/Object.h"

#include "LRMenuWidgetController.generated.h"

class AActor;
class ULRInventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLRMenuScreenChanged, ELRScreenType, previousScreen, ELRScreenType, currentScreen);

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Menu Widget Controller"))
class LOSTRUNIC_API ULRMenuWidgetController : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * @brief 切换到指定菜单页面并广播变化；同一时刻只保留一个可见菜单。
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
	 * @param inventory 参与本次操作的运行时对象 `inventory`；函数会检查空值和所需接口。
	 * @param itemUseTarget 本次规则检查或操作的目标对象；交互选物模式下传入，普通浏览传 nullptr。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|UI")
	FLRInventorySnapshot BuildInventorySnapshot(const ULRInventoryComponent* inventory, AActor* itemUseTarget = nullptr) const;

	/**
	 * @brief 查询 Open Screen；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	ELRScreenType GetOpenScreen() const { return OpenScreenType; }

	/** 当 Menu Screen Changed 发生时广播；蓝图可绑定该委托以更新表现，不应在回调中改写核心规则。  */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|UI")
	FLRMenuScreenChanged OnMenuScreenChanged;

private:
	/** Open Screen Type 的内部运行时数据；不参与蓝图配置。 */
	ELRScreenType OpenScreenType = ELRScreenType::None;
};
