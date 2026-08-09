/**
 * @file LRItemTests.cpp
 * @brief 提供 LostRunic Runtime 自动化测试，覆盖调优边界、状态矩阵、交互筛选、物品双入口、守卫警戒、叙事分支和存档事务顺序。仅在 WITH_DEV_AUTOMATION_TESTS 下编译。
 *
 * 关联文件：Tests 目录内调用该公共契约的实现文件；所属领域：Tests。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/LRGameplayTags.h"
#include "Data/LRItemDefinition.h"
#include "Items/LRInventoryComponent.h"
#include "Tests/LRTestItemUseTargetComponent.h"

namespace
{
	/**
	 * @brief 根据当前领域状态构建 Make Item 所需的数据，不把临时对象作为长期存档标识。
	 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
	 * @param bConsumable 布尔开关 `bConsumable`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 * @param bCourage 布尔开关 `bCourage`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	ULRItemDefinition* MakeItem(const FName itemId, const bool bConsumable, const bool bCourage = false)
	{
		ULRItemDefinition* definition = NewObject<ULRItemDefinition>();
		definition->ItemId = itemId;
		definition->bConsumable = bConsumable;
		definition->AllowedActionTags.AddTag(LRGameplayTags::InteractionActionUse);
		definition->AllowedTargetTags.AddTag(LRGameplayTags::TargetGuardCourageVulnerable);
		if (bCourage)
		{
			definition->ItemTags.AddTag(LRGameplayTags::ItemCategoryCourageWeapon);
		}
		return definition;
	}

	/**
	 * @brief 根据当前领域状态构建 Make Inventory 所需的数据，不把临时对象作为长期存档标识。
	 * @param definition 数据或调优来源 `definition`；调用期间只读，并按稳定 ID 解析内容。
	 * @param count 本次操作使用的计数、增量或索引 `count`；由函数校验合法范围。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	ULRInventoryComponent* MakeInventory(ULRItemDefinition* definition, const int32 count = 1)
	{
		ULRInventoryComponent* inventory = NewObject<ULRInventoryComponent>();
		inventory->InitializeDefinitions({ definition });
		inventory->AddItem(definition->ItemId, count);
		return inventory;
	}

	/**
	 * @brief 根据当前领域状态构建 Make Target 所需的数据，不把临时对象作为长期存档标识。
	 * @param tag Gameplay Tag 或标签集合，用于分类、条件、拒绝原因和可诊断事件。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	ULRTestItemUseTargetComponent* MakeTarget(const FGameplayTag tag)
	{
		ULRTestItemUseTargetComponent* target = NewObject<ULRTestItemUseTargetComponent>();
		target->TargetTags.AddTag(tag);
		return target;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRQuickSlotInventoryTest, "LostRunic.Item.QuickSlotsHaveFixedFourSlotSemantics",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRQuickSlotInventoryTest::RunTest(const FString& parameters)
{
	ULRItemDefinition* definition = MakeItem(TEXT("Test.Key"), false);
	ULRInventoryComponent* inventory = MakeInventory(definition);
	TestTrue(TEXT("First slot accepts an owned item"), inventory->AssignQuickSlot(0, definition->ItemId));
	TestTrue(TEXT("Fourth slot accepts an owned item"), inventory->AssignQuickSlot(3, definition->ItemId));
	TestFalse(TEXT("Fifth slot is rejected"), inventory->AssignQuickSlot(4, definition->ItemId));
	inventory->SelectQuickSlot(3);
	inventory->SelectAdjacentQuickSlot(1);
	TestEqual(TEXT("Selection wraps from fourth to first"), inventory->GetSelectedQuickSlot(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRItemEntryPointTest, "LostRunic.Item.QuickSlotAndSelectorShareResolver",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRItemEntryPointTest::RunTest(const FString& parameters)
{
	ULRItemDefinition* definition = MakeItem(TEXT("Test.Key"), false);
	ULRInventoryComponent* inventory = MakeInventory(definition);
	ULRTestItemUseTargetComponent* target = MakeTarget(LRGameplayTags::TargetGuardCourageVulnerable);
	inventory->AssignQuickSlot(0, definition->ItemId);
	const FLRItemUseRequest quickRequest = inventory->BuildUseRequest(definition->ItemId, 0, target,
		ELRPerceptionMode::Normal, ELRItemUseEntryPoint::QuickSlot);
	const FLRItemUseRequest selectorRequest = inventory->BuildUseRequest(definition->ItemId, INDEX_NONE, target,
		ELRPerceptionMode::Normal, ELRItemUseEntryPoint::InteractionSelector);

	TestTrue(TEXT("Quick slot succeeds"), inventory->ResolveUseRequestAtTime(quickRequest, 1.0).bSuccess);
	TestTrue(TEXT("Selector succeeds through the same resolver"), inventory->ResolveUseRequestAtTime(selectorRequest, 1.0).bSuccess);
	TestEqual(TEXT("Both entries reached one target contract"), target->ApplyCount, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRItemRollbackTest, "LostRunic.Item.FailedConsumptionRollsBack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRItemRollbackTest::RunTest(const FString& parameters)
{
	ULRItemDefinition* definition = MakeItem(TEXT("Test.Consumable"), true);
	ULRInventoryComponent* inventory = MakeInventory(definition);
	ULRTestItemUseTargetComponent* target = MakeTarget(LRGameplayTags::TargetGuardCourageVulnerable);
	target->bShouldSucceed = false;
	const FLRItemUseRequest request = inventory->BuildUseRequest(definition->ItemId, INDEX_NONE, target,
		ELRPerceptionMode::Normal, ELRItemUseEntryPoint::InteractionSelector);
	const FLRItemUseResult result = inventory->ResolveUseRequestAtTime(request, 1.0);
	TestFalse(TEXT("Target failure is returned"), result.bSuccess);
	TestFalse(TEXT("Rolled-back transaction does not report consumption"), result.bConsumed);
	TestEqual(TEXT("Consumed item is restored"), inventory->GetItemCount(definition->ItemId), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRCourageItemTest, "LostRunic.Item.CourageStateCooldownAndImmunity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRCourageItemTest::RunTest(const FString& parameters)
{
	ULRItemDefinition* definition = MakeItem(TEXT("Test.Courage"), false, true);
	definition->AllowedTargetTags.AddTag(LRGameplayTags::TargetGuardCourageImmune);
	ULRInventoryComponent* inventory = MakeInventory(definition);
	ULRTestItemUseTargetComponent* vulnerable = MakeTarget(LRGameplayTags::TargetGuardCourageVulnerable);
	FLRItemUseRequest request = inventory->BuildUseRequest(definition->ItemId, INDEX_NONE, vulnerable,
		ELRPerceptionMode::Normal, ELRItemUseEntryPoint::InteractionSelector);
	TestTrue(TEXT("Normal mode is rejected"), inventory->ResolveUseRequestAtTime(request, 10.0).FailureReason
		== LRGameplayTags::InteractionRejectState);
	request.CurrentMode = ELRPerceptionMode::Courage;
	TestTrue(TEXT("First Courage use succeeds"), inventory->ResolveUseRequestAtTime(request, 10.0).bSuccess);
	TestTrue(TEXT("Use inside one-second cooldown is rejected"), inventory->ResolveUseRequestAtTime(request, 10.5).FailureReason
		== LRGameplayTags::ItemUseRejectCooldown);
	TestTrue(TEXT("Use at cooldown boundary succeeds"), inventory->ResolveUseRequestAtTime(request, 11.0).bSuccess);

	ULRInventoryComponent* immuneInventory = MakeInventory(definition);
	request = immuneInventory->BuildUseRequest(definition->ItemId, INDEX_NONE,
		MakeTarget(LRGameplayTags::TargetGuardCourageImmune), ELRPerceptionMode::Courage,
		ELRItemUseEntryPoint::InteractionSelector);
	TestTrue(TEXT("Immune target has a structured rejection"),
		immuneInventory->ResolveUseRequestAtTime(request, 10.0).FailureReason == LRGameplayTags::ItemUseRejectImmune);
	return true;
}

#endif
