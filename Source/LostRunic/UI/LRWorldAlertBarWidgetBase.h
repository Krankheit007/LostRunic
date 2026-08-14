/**
 * @file LRWorldAlertBarWidgetBase.h
 * @brief 世界空间警戒条 Widget 基类：由守卫初始化并绑定/解绑 ULRAlertComponent 的只读警戒快照，绑定后立即推送一次快照；蓝图只负责表现（0 隐藏 / 1-5 白 / 6-10 红 / 11 满值特效）。
 *
 * 关联文件：LRWorldAlertBarWidgetBase.cpp；所属领域：UI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "AI/LRGuardTypes.h"
#include "Blueprint/UserWidget.h"

#include "LRWorldAlertBarWidgetBase.generated.h"

class ALRGuardCharacter;
class ULRAlertComponent;

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(Abstract, BlueprintType, meta = (DisplayName = "Lost Runic World Alert Bar Base"))
class LOSTRUNIC_API ULRWorldAlertBarWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * @brief 绑定指定守卫的警戒快照并立即推送当前值，避免首帧不同步；重复调用会先解绑旧守卫。
	 * @param guard 本次查询、交互或事件涉及的 Actor。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|UI|Alert")
	void InitializeForGuard(ALRGuardCharacter* guard);

	/**
	 * @brief 解绑当前守卫的警戒快照；Widget 销毁时自动调用。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|UI|Alert")
	void Shutdown();

	/**
	 * @brief 查询 Current Snapshot；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|UI|Alert")
	const FLRAlertSnapshot& GetCurrentSnapshot() const { return CurrentSnapshot; }

	/**
	 * @brief 警戒快照变化时调用；蓝图覆盖此事件只做表现（进度条、颜色、隐藏与满值特效）。
	 * @param snapshot 本次领域操作的结构化数据 `snapshot`；字段语义由对应 USTRUCT 定义。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Lost Runic|UI|Alert")
	void HandleAlertSnapshotChanged(const FLRAlertSnapshot& snapshot);

protected:
	/**
	 * @brief Widget 销毁时解绑警戒快照，避免悬挂委托。
	 */
	virtual void NativeDestruct() override;

private:
	/**
	 * @brief 处理 Handle Snapshot Changed 事件，将引擎回调转换为对应领域状态更新。
	 * @param snapshot 本次领域操作的结构化数据 `snapshot`；字段语义由对应 USTRUCT 定义。
	 */
	UFUNCTION()
	void HandleSnapshotChanged(const FLRAlertSnapshot& snapshot);

	/** Alert 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TWeakObjectPtr<ULRAlertComponent> Alert;

	/** Current Snapshot 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	FLRAlertSnapshot CurrentSnapshot;
};
