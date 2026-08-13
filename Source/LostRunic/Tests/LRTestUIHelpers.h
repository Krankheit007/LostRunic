/**
 * @file LRTestUIHelpers.h
 * @brief 提供 UI 自动化测试使用的具体 Screen 子类与委托观察者：验证通用命令路由不依赖 Inventory 类型、快照广播与领域事件计数。
 *
 * 关联文件：LRUITests.cpp；所属领域：Tests。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 */
#pragma once

#include "UI/LRScreenWidget.h"
#include "UI/LRUITypes.h"
#include "UObject/Object.h"

#include "LRTestUIHelpers.generated.h"

/** 具体化抽象基类，供命令路由测试实例化。 */
UCLASS(meta = (DisplayName = "LR Test Screen Widget"))
class ULRTestScreenWidget : public ULRScreenWidget
{
	GENERATED_BODY()
};

/** 统计 OnSnapshotChanged 广播次数并保留最近一次快照。 */
UCLASS()
class ULRTestSnapshotObserver : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void HandleSnapshotChanged(const FLRInventorySnapshot& snapshot)
	{
		++BroadcastCount;
		LastSnapshot = snapshot;
	}

	int32 BroadcastCount = 0;
	FLRInventorySnapshot LastSnapshot;
};

/** 统计笔记/收藏品/武器选择领域事件广播次数。 */
UCLASS()
class ULRTestInventoryObserver : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void HandleNotesChanged() { ++NotesChangedCount; }

	UFUNCTION()
	void HandleCollectiblesChanged() { ++CollectiblesChangedCount; }

	UFUNCTION()
	void HandleSelectedWeaponChanged() { ++SelectedWeaponChangedCount; }

	int32 NotesChangedCount = 0;
	int32 CollectiblesChangedCount = 0;
	int32 SelectedWeaponChangedCount = 0;
};
