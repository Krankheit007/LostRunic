/**
 * @file LRScreenWidget.h
 * @brief 实现 HUD、状态遮罩、对话/阅读、背包/笔记/收藏、暂停、存档槽和过场的控制器边界。UI 订阅领域事件并负责表现，不参与核心规则判定。
 *
 * 关联文件：LRScreenWidget.cpp；所属领域：UI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Blueprint/UserWidget.h"
#include "UI/LRUITypes.h"

#include "LRScreenWidget.generated.h"

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(Abstract, Blueprintable, meta = (DisplayName = "Lost Runic Screen Widget"))
class LOSTRUNIC_API ULRScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * @brief 更新 Screen Visible，并在需要时同步组件状态或广播变化事件。
	 * @param bVisible 布尔开关 `bVisible`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 */
	void SetScreenVisible(bool bVisible);
	/**
	 * @brief 把当前叙事页面数据推送到 Widget 表现，不执行剧情条件或存档规则。
	 * @param presentation 本次领域操作的结构化数据 `presentation`；字段语义由对应 USTRUCT 定义。
	 */
	void PresentNarrative(const FLRNarrativePresentation& presentation);

	/**
	 * @brief 查询 Screen Type；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	ELRScreenType GetScreenType() const { return ScreenType; }
	/**
	 * @brief 判断 Is Screen Visible 对应条件；不产生玩法副作用。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool IsScreenVisible() const { return bScreenVisible; }

	/**
	 * @brief 处理 On Screen Visibility Changed 事件，将引擎回调转换为对应领域状态更新。
	 * @param bVisible 布尔开关 `bVisible`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Lost Runic|UI")
	void OnScreenVisibilityChanged(bool bVisible);

	/**
	 * @brief 处理 On Narrative Presentation Changed 事件，将引擎回调转换为对应领域状态更新。
	 * @param presentation 本次领域操作的结构化数据 `presentation`；字段语义由对应 USTRUCT 定义。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Lost Runic|UI")
	void OnNarrativePresentationChanged(const FLRNarrativePresentation& presentation);

protected:
	/**
	 * @brief 在 UMG 原生初始化阶段建立 Widget 自身状态；领域事件由外部控制器绑定。
	 */
	virtual void NativeOnInitialized() override;

	/** Screen Type 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `ELRScreenType::None`。 可在 DataAsset 或蓝图类默认值中配置，运行时蓝图只读。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Screen")
	ELRScreenType ScreenType = ELRScreenType::None;

private:
	/** Screen Visible 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	bool bScreenVisible = false;
};
