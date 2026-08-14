/**
 * @file LRWorldAlertBarWidgetBase.cpp
 * @brief 世界空间警戒条 Widget 基类实现：守卫初始化时绑定警戒快照，Widget 销毁时解绑；初始快照在绑定后立即推送。
 *
 * 关联文件：LRWorldAlertBarWidgetBase.h；所属领域：UI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "UI/LRWorldAlertBarWidgetBase.h"

#include "AI/LRAlertComponent.h"
#include "AI/LRGuardCharacter.h"

/**
 * @brief 绑定指定守卫的警戒快照并立即推送当前值，避免首帧不同步；重复调用会先解绑旧守卫。
 * @param guard 本次查询、交互或事件涉及的 Actor。
 */
void ULRWorldAlertBarWidgetBase::InitializeForGuard(ALRGuardCharacter* guard)
{
	Shutdown();
	Alert = guard ? guard->GetAlertComponent() : nullptr;
	if (Alert.IsValid())
	{
		Alert->OnAlertSnapshotChanged.AddDynamic(this, &ULRWorldAlertBarWidgetBase::HandleSnapshotChanged);
		HandleSnapshotChanged(Alert->GetAlertSnapshot());
	}
}

/**
 * @brief 解绑当前守卫的警戒快照；Widget 销毁时自动调用。
 */
void ULRWorldAlertBarWidgetBase::Shutdown()
{
	if (Alert.IsValid())
	{
		Alert->OnAlertSnapshotChanged.RemoveDynamic(this, &ULRWorldAlertBarWidgetBase::HandleSnapshotChanged);
	}
	Alert.Reset();
}

/**
 * @brief Widget 销毁时解绑警戒快照，避免悬挂委托。
 */
void ULRWorldAlertBarWidgetBase::NativeDestruct()
{
	Shutdown();
	Super::NativeDestruct();
}

/**
 * @brief 处理 Handle Snapshot Changed 事件，将引擎回调转换为对应领域状态更新。
 * @param snapshot 本次领域操作的结构化数据 `snapshot`；字段语义由对应 USTRUCT 定义。
 */
void ULRWorldAlertBarWidgetBase::HandleSnapshotChanged(const FLRAlertSnapshot& snapshot)
{
	CurrentSnapshot = snapshot;
	HandleAlertSnapshotChanged(snapshot);
}
