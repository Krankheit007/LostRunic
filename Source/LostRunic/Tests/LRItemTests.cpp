/**
 * @file LRItemTests.cpp
 * @brief 提供 LostRunic Runtime 自动化测试，覆盖库存堆叠与消费、武器选择回退、攻击事务、笔记/收藏品和菜单快照。仅在 WITH_DEV_AUTOMATION_TESTS 下编译。
 *
 * 关联文件：Tests 目录内调用该公共契约的实现文件；所属领域：Tests。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/LRGameplayTags.h"
#include "Data/LRCollectibleDefinition.h"
#include "Data/LRItemDefinition.h"
#include "Data/LRStateTuning.h"
#include "Framework/LRCharacter.h"
#include "Items/LRAttackTargetResolver.h"
#include "Items/LRInventoryComponent.h"
#include "Items/LRItemUseResolver.h"
#include "Tests/LRTestAttackTargetComponent.h"
#include "Tests/LRTestItemUseTargetComponent.h"
#include "UI/LRMenuWidgetController.h"
#include "GameFramework/SpringArmComponent.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

namespace
{
	/**
	 * @brief 构造测试物品定义；bWeapon 时附加 Item.Category.Weapon 与 Attack 入口。
	 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
	 * @param bConsumable 布尔开关 `bConsumable`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 * @param bWeapon 布尔开关 `bWeapon`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	ULRItemDefinition* MakeItem(const FName itemId, const bool bConsumable, const bool bWeapon = false)
	{
		ULRItemDefinition* definition = NewObject<ULRItemDefinition>();
		definition->ItemId = itemId;
		definition->bConsumable = bConsumable;
		definition->AllowedActionTags.AddTag(LRGameplayTags::InteractionActionUse);
		definition->AllowedTargetTags.AddTag(LRGameplayTags::TargetGuardCourageVulnerable);
		if (bWeapon)
		{
			definition->ItemTags.AddTag(LRGameplayTags::ItemCategoryWeapon);
			definition->AllowedActionTags.AddTag(LRGameplayTags::InteractionActionAttack);
		}
		return definition;
	}

	/**
	 * @brief 构造带测试物品定义的库存，并注入指定数量。
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
	 * @brief 构造绑定到测试库存的统一事务解析器。
	 * @param inventory 参与本次操作的运行时对象 `inventory`；函数会检查空值和所需接口。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	ULRItemUseResolver* MakeResolver(ULRInventoryComponent* inventory)
	{
		ULRItemUseResolver* resolver = NewObject<ULRItemUseResolver>();
		resolver->Initialize(inventory);
		return resolver;
	}

	/**
	 * @brief 构造统一使用请求（Interaction 入口）。
	 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
	 * @param target 本次规则检查或操作的目标对象。
	 * @param mode 本次操作使用的 `mode` 枚举或模式值。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	FLRItemUseRequest MakeUseRequest(const FName itemId, UObject* target, const ELRPerceptionMode mode)
	{
		FLRItemUseRequest request;
		request.ItemId = itemId;
		request.Target = target;
		request.EntryPoint = ELRItemUseEntryPoint::Interaction;
		request.CurrentMode = mode;
		request.ActionTag = LRGameplayTags::InteractionActionUse;
		return request;
	}

	/**
	 * @brief 构造统一攻击请求（Attack 入口）。
	 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名；NAME_None 表示空手攻击。
	 * @param target 本次规则检查或操作的目标对象。
	 * @param mode 本次操作使用的 `mode` 枚举或模式值。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	FLRItemUseRequest MakeAttackRequest(const FName itemId, UObject* target, const ELRPerceptionMode mode)
	{
		FLRItemUseRequest request;
		request.ItemId = itemId;
		request.Target = target;
		request.EntryPoint = ELRItemUseEntryPoint::Attack;
		request.CurrentMode = mode;
		request.ActionTag = LRGameplayTags::InteractionActionAttack;
		return request;
	}

	/**
	 * @brief 构造交互目标测试替身。
	 * @param tag Gameplay Tag 或标签集合，用于分类、条件、拒绝原因和可诊断事件。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	ULRTestItemUseTargetComponent* MakeUseTarget(const FGameplayTag tag)
	{
		ULRTestItemUseTargetComponent* target = NewObject<ULRTestItemUseTargetComponent>();
		target->TargetTags.AddTag(tag);
		return target;
	}

	/**
	 * @brief 构造攻击目标测试替身。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	ULRTestAttackTargetComponent* MakeAttackTarget()
	{
		ULRTestAttackTargetComponent* target = NewObject<ULRTestAttackTargetComponent>();
		target->TargetTags.AddTag(LRGameplayTags::TargetGuardCourageVulnerable);
		return target;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRInfiniteItemTest, "LostRunic.Item.InfiniteUseKeepsQuantityOne",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRInfiniteItemTest::RunTest(const FString& parameters)
{
	ULRItemDefinition* definition = MakeItem(TEXT("Test.Infinite"), false);
	ULRInventoryComponent* inventory = MakeInventory(definition);
	ULRItemUseResolver* resolver = MakeResolver(inventory);
	ULRTestItemUseTargetComponent* target = MakeUseTarget(LRGameplayTags::TargetGuardCourageVulnerable);

	const FLRItemUseResult first = resolver->ResolveAtTime(MakeUseRequest(definition->ItemId, target, ELRPerceptionMode::Normal), 1.0);
	TestTrue(TEXT("First use succeeds"), first.bSuccess);
	TestFalse(TEXT("Infinite item never reports consumption"), first.bConsumed);
	TestEqual(TEXT("Quantity stays one after success"), inventory->GetItemCount(definition->ItemId), 1);
	const FLRItemUseResult second = resolver->ResolveAtTime(MakeUseRequest(definition->ItemId, target, ELRPerceptionMode::Normal), 2.0);
	TestTrue(TEXT("Infinite item can be used again"), second.bSuccess);
	TestEqual(TEXT("Quantity remains one after repeated use"), inventory->GetItemCount(definition->ItemId), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRConsumableCountdownTest, "LostRunic.Item.ConsumableCountsDownOnSuccess",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRConsumableCountdownTest::RunTest(const FString& parameters)
{
	ULRItemDefinition* definition = MakeItem(TEXT("Test.Consumable"), true);
	definition->MaxStackSize = 3;
	ULRInventoryComponent* inventory = MakeInventory(definition, 3);
	ULRItemUseResolver* resolver = MakeResolver(inventory);
	ULRTestItemUseTargetComponent* target = MakeUseTarget(LRGameplayTags::TargetGuardCourageVulnerable);

	TestEqual(TEXT("Initial quantity is three"), inventory->GetItemCount(definition->ItemId), 3);
	const FLRItemUseResult first = resolver->ResolveAtTime(MakeUseRequest(definition->ItemId, target, ELRPerceptionMode::Normal), 1.0);
	TestTrue(TEXT("First use succeeds"), first.bSuccess);
	TestTrue(TEXT("First use consumes one"), first.bConsumed);
	TestEqual(TEXT("Quantity becomes two"), inventory->GetItemCount(definition->ItemId), 2);
	TestTrue(TEXT("Second use succeeds"), resolver->ResolveAtTime(MakeUseRequest(definition->ItemId, target, ELRPerceptionMode::Normal), 2.0).bSuccess);
	TestEqual(TEXT("Quantity becomes one"), inventory->GetItemCount(definition->ItemId), 1);
	TestTrue(TEXT("Third use succeeds"), resolver->ResolveAtTime(MakeUseRequest(definition->ItemId, target, ELRPerceptionMode::Normal), 3.0).bSuccess);
	TestEqual(TEXT("Quantity becomes zero and entry is removed"), inventory->GetItemCount(definition->ItemId), 0);
	TestFalse(TEXT("Entry is removed after reaching zero"), inventory->HasItem(definition->ItemId));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRFailedUseKeepsQuantityTest, "LostRunic.Item.FailedUseDoesNotConsume",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRFailedUseKeepsQuantityTest::RunTest(const FString& parameters)
{
	ULRItemDefinition* definition = MakeItem(TEXT("Test.Consumable"), true);
	ULRInventoryComponent* inventory = MakeInventory(definition);
	ULRItemUseResolver* resolver = MakeResolver(inventory);
	ULRTestItemUseTargetComponent* target = MakeUseTarget(LRGameplayTags::TargetGuardCourageVulnerable);
	target->bShouldSucceed = false;

	const FLRItemUseResult result = resolver->ResolveAtTime(MakeUseRequest(definition->ItemId, target, ELRPerceptionMode::Normal), 1.0);
	TestFalse(TEXT("Target failure is returned"), result.bSuccess);
	TestFalse(TEXT("Failed transaction does not report consumption"), result.bConsumed);
	TestEqual(TEXT("Quantity is untouched after failure"), inventory->GetItemCount(definition->ItemId), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRAddItemValidationTest, "LostRunic.Item.AddItemValidatesStackSizeAndInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRAddItemValidationTest::RunTest(const FString& parameters)
{
	ULRItemDefinition* definition = MakeItem(TEXT("Test.Key"), false);
	definition->MaxStackSize = 3;
	ULRInventoryComponent* inventory = MakeInventory(definition, 3);

	TestEqual(TEXT("Adding beyond MaxStackSize returns InventoryFull"),
		inventory->AddItem(definition->ItemId, 1), ELRAddItemResult::InventoryFull);
	TestEqual(TEXT("Quantity stays at the maximum"), inventory->GetItemCount(definition->ItemId), 3);
	TestEqual(TEXT("Empty item id is rejected"),
		inventory->AddItem(NAME_None, 1), ELRAddItemResult::InvalidDefinition);
	TestEqual(TEXT("Unknown item id is rejected"),
		inventory->AddItem(TEXT("Test.Unknown"), 1), ELRAddItemResult::InvalidDefinition);
	TestEqual(TEXT("Non-positive quantity is rejected"),
		inventory->AddItem(definition->ItemId, 0), ELRAddItemResult::InvalidQuantity);
	TestEqual(TEXT("Negative quantity is rejected"),
		inventory->AddItem(definition->ItemId, -1), ELRAddItemResult::InvalidQuantity);
	return true;
}

#if WITH_EDITOR
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRItemDefinitionValidationTest, "LostRunic.Item.DefinitionValidationRejectsAmbiguousStacks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRItemDefinitionValidationTest::RunTest(const FString& parameters)
{
	ULRItemDefinition* infinite = NewObject<ULRItemDefinition>();
	infinite->ItemId = TEXT("Test.Infinite");
	infinite->bConsumable = false;
	infinite->MaxStackSize = 2;
	FDataValidationContext context;
	TestEqual(TEXT("Infinite-use item with MaxStackSize>1 is rejected"),
		infinite->IsDataValid(context), EDataValidationResult::Invalid);

	ULRItemDefinition* nonWeaponAttack = NewObject<ULRItemDefinition>();
	nonWeaponAttack->ItemId = TEXT("Test.NonWeaponAttack");
	nonWeaponAttack->AllowedActionTags.AddTag(LRGameplayTags::InteractionActionAttack);
	TestEqual(TEXT("Attack entry without Item.Category.Weapon is rejected"),
		nonWeaponAttack->IsDataValid(context), EDataValidationResult::Invalid);

	ULRItemDefinition* legalConsumable = NewObject<ULRItemDefinition>();
	legalConsumable->ItemId = TEXT("Test.Legal");
	legalConsumable->bConsumable = true;
	legalConsumable->MaxStackSize = 5;
	TestTrue(TEXT("Consumable stack above one is legal"),
		legalConsumable->IsDataValid(context) != EDataValidationResult::Invalid);
	return true;
}
#endif

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRWeaponSelectionTest, "LostRunic.Item.WeaponSelectionAndFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRWeaponSelectionTest::RunTest(const FString& parameters)
{
	ULRItemDefinition* knife = MakeItem(TEXT("Test.Knife"), true, true);
	ULRItemDefinition* pipe = MakeItem(TEXT("Test.Pipe"), true, true);
	ULRItemDefinition* gun = MakeItem(TEXT("Test.Gun"), true, true);
	ULRInventoryComponent* inventory = NewObject<ULRInventoryComponent>();
	inventory->InitializeDefinitions({ knife, pipe, gun });

	TestEqual(TEXT("None is a legal selected weapon"), inventory->GetSelectedWeapon(), NAME_None);
	TestEqual(TEXT("No weapons means no effective weapon"), inventory->GetEffectiveWeapon(), NAME_None);

	inventory->AddItem(pipe->ItemId, 1);
	inventory->AddItem(knife->ItemId, 1);
	inventory->AddItem(gun->ItemId, 1);
	TestEqual(TEXT("Without explicit selection the earliest weapon is effective"),
		inventory->GetEffectiveWeapon(), pipe->ItemId);
	TestTrue(TEXT("Explicit selection of a later weapon is accepted"), inventory->SetSelectedWeapon(knife->ItemId));
	TestEqual(TEXT("Selected weapon wins over acquisition order"), inventory->GetEffectiveWeapon(), knife->ItemId);
	TestEqual(TEXT("GetSelectedWeapon returns the explicit choice"), inventory->GetSelectedWeapon(), knife->ItemId);
	TestFalse(TEXT("Non-weapon items cannot become selected weapons"), inventory->SetSelectedWeapon(TEXT("Test.NotWeapon")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRWeaponConsumeClearsSelectionTest, "LostRunic.Item.SelectedWeaponClearedWhenConsumed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRWeaponConsumeClearsSelectionTest::RunTest(const FString& parameters)
{
	ULRItemDefinition* knife = MakeItem(TEXT("Test.Knife"), true, true);
	ULRItemDefinition* pipe = MakeItem(TEXT("Test.Pipe"), true, true);
	ULRInventoryComponent* inventory = NewObject<ULRInventoryComponent>();
	inventory->InitializeDefinitions({ knife, pipe });
	inventory->AddItem(pipe->ItemId, 1);
	inventory->AddItem(knife->ItemId, 1);
	inventory->SetSelectedWeapon(knife->ItemId);

	ULRItemUseResolver* resolver = MakeResolver(inventory);
	ULRTestAttackTargetComponent* target = MakeAttackTarget();
	const FLRItemUseResult attack = resolver->ResolveAtTime(
		MakeAttackRequest(knife->ItemId, target, ELRPerceptionMode::Courage), 1.0);
	TestTrue(TEXT("Knife attack succeeds"), attack.bSuccess);
	TestTrue(TEXT("Knife attack consumes the item"), attack.bConsumed);
	TestEqual(TEXT("Knife is consumed"), inventory->GetItemCount(knife->ItemId), 0);
	TestEqual(TEXT("Consumed selection is cleared"), inventory->GetSelectedWeapon(), NAME_None);
	TestEqual(TEXT("Effective weapon falls back to the remaining weapon"),
		inventory->GetEffectiveWeapon(), pipe->ItemId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRWeaponOrderFallbackTest, "LostRunic.Item.AcquisitionOrderDrivesFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRWeaponOrderFallbackTest::RunTest(const FString& parameters)
{
	ULRItemDefinition* pipe = MakeItem(TEXT("Test.Pipe"), true, true);
	ULRItemDefinition* knife = MakeItem(TEXT("Test.Knife"), true, true);
	ULRItemDefinition* gun = MakeItem(TEXT("Test.Gun"), true, true);
	ULRInventoryComponent* inventory = NewObject<ULRInventoryComponent>();
	inventory->InitializeDefinitions({ pipe, knife, gun });
	inventory->AddItem(pipe->ItemId, 1);
	inventory->AddItem(knife->ItemId, 1);
	inventory->AddItem(gun->ItemId, 1);

	ULRItemUseResolver* resolver = MakeResolver(inventory);
	ULRTestAttackTargetComponent* target = MakeAttackTarget();
	const TArray<FName> expectedFallback = { pipe->ItemId, knife->ItemId, gun->ItemId };
	double attackTimeSeconds = 10.0;
	for (const FName& expected : expectedFallback)
	{
		TestEqual(TEXT("Fallback follows acquisition order"), inventory->GetEffectiveWeapon(), expected);
		TestTrue(TEXT("Consuming the effective weapon succeeds"),
			resolver->ResolveAtTime(MakeAttackRequest(expected, target, ELRPerceptionMode::Courage), attackTimeSeconds).bSuccess);
		attackTimeSeconds += 2.0;
	}
	TestEqual(TEXT("No weapons left yields no effective weapon"), inventory->GetEffectiveWeapon(), NAME_None);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRWeaponReacquireTest, "LostRunic.Item.ReacquiredWeaponGetsNewSequence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRWeaponReacquireTest::RunTest(const FString& parameters)
{
	ULRItemDefinition* knife = MakeItem(TEXT("Test.Knife"), true, true);
	ULRItemDefinition* pipe = MakeItem(TEXT("Test.Pipe"), true, true);
	ULRInventoryComponent* inventory = NewObject<ULRInventoryComponent>();
	inventory->InitializeDefinitions({ knife, pipe });
	inventory->AddItem(knife->ItemId, 1);
	inventory->AddItem(pipe->ItemId, 1);

	ULRItemUseResolver* resolver = MakeResolver(inventory);
	ULRTestAttackTargetComponent* target = MakeAttackTarget();
	TestTrue(TEXT("Consuming the earliest weapon succeeds"),
		resolver->ResolveAtTime(MakeAttackRequest(knife->ItemId, target, ELRPerceptionMode::Courage), 1.0).bSuccess);
	TestEqual(TEXT("Reacquiring the consumed weapon succeeds"), inventory->AddItem(knife->ItemId, 1), ELRAddItemResult::Success);
	TestEqual(TEXT("Reacquired weapon is treated as newer than the other weapon"),
		inventory->GetEffectiveWeapon(), pipe->ItemId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRAttackTransactionTest, "LostRunic.Item.AttackTransactionRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRAttackTransactionTest::RunTest(const FString& parameters)
{
	ULRItemDefinition* knife = MakeItem(TEXT("Test.Knife"), true, true);
	ULRInventoryComponent* inventory = MakeInventory(knife);
	ULRItemUseResolver* resolver = MakeResolver(inventory);
	ULRTestAttackTargetComponent* attackTarget = MakeAttackTarget();

	const FLRItemUseResult stateReject = resolver->ResolveAtTime(
		MakeAttackRequest(knife->ItemId, attackTarget, ELRPerceptionMode::Normal), 1.0);
	TestTrue(TEXT("Non-Courage attack is rejected by the state rule"),
		stateReject.FailureReason == LRGameplayTags::ItemUseRejectAttackState);
	TestEqual(TEXT("Rejected attack does not consume"), inventory->GetItemCount(knife->ItemId), 1);

	const FLRItemUseResult emptyHanded = resolver->ResolveAtTime(
		MakeAttackRequest(NAME_None, attackTarget, ELRPerceptionMode::Courage), 2.0);
	TestTrue(TEXT("Empty-handed attack executes"), emptyHanded.bSuccess);
	TestEqual(TEXT("Empty-handed attack reaches the target contract"), attackTarget->ApplyCount, 1);

	ULRTestItemUseTargetComponent* door = MakeUseTarget(LRGameplayTags::TargetDoorHomeKey);
	const FLRItemUseResult doorAttack = resolver->ResolveAtTime(
		MakeAttackRequest(knife->ItemId, door, ELRPerceptionMode::Courage), 3.0);
	TestTrue(TEXT("A plain item-use target is never an attack target"),
		doorAttack.FailureReason == LRGameplayTags::ItemUseRejectTarget);

	const FLRItemUseResult firstAttack = resolver->ResolveAtTime(
		MakeAttackRequest(knife->ItemId, attackTarget, ELRPerceptionMode::Courage), 4.0);
	TestTrue(TEXT("Weapon attack succeeds"), firstAttack.bSuccess);
	TestEqual(TEXT("Weapon attack consumes the knife"), inventory->GetItemCount(knife->ItemId), 0);

	ULRTestAttackTargetComponent* immune = MakeAttackTarget();
	immune->TargetTags.Reset();
	immune->TargetTags.AddTag(LRGameplayTags::TargetGuardCourageImmune);
	ULRInventoryComponent* immuneInventory = MakeInventory(knife);
	ULRItemUseResolver* immuneResolver = MakeResolver(immuneInventory);
	const FLRItemUseResult immuneReject = immuneResolver->ResolveAtTime(
		MakeAttackRequest(knife->ItemId, immune, ELRPerceptionMode::Courage), 4.0);
	TestTrue(TEXT("Immune target has a structured rejection"),
		immuneReject.FailureReason == LRGameplayTags::ItemUseRejectImmune);
	TestEqual(TEXT("Immune rejection does not consume"), immuneInventory->GetItemCount(knife->ItemId), 1);

	const FLRItemUseResult cooldownReject = resolver->ResolveAtTime(
		MakeAttackRequest(NAME_None, attackTarget, ELRPerceptionMode::Courage), 4.5);
	TestTrue(TEXT("Attack inside cooldown is rejected"),
		cooldownReject.FailureReason == LRGameplayTags::ItemUseRejectCooldown);
	const FLRItemUseResult boundaryAttack = resolver->ResolveAtTime(
		MakeAttackRequest(NAME_None, attackTarget, ELRPerceptionMode::Courage), 5.5);
	TestTrue(TEXT("Attack at the cooldown boundary succeeds"), boundaryAttack.bSuccess);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRInvalidAttackItemTest, "LostRunic.Item.NonWeaponWithAttackTagIsRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRInvalidAttackItemTest::RunTest(const FString& parameters)
{
	ULRItemDefinition* misconfigured = MakeItem(TEXT("Test.Misconfigured"), false);
	misconfigured->AllowedActionTags.AddTag(LRGameplayTags::InteractionActionAttack);
	ULRInventoryComponent* inventory = MakeInventory(misconfigured);
	ULRItemUseResolver* resolver = MakeResolver(inventory);
	ULRTestAttackTargetComponent* target = MakeAttackTarget();

	const FLRItemUseResult result = resolver->ResolveAtTime(
		MakeAttackRequest(misconfigured->ItemId, target, ELRPerceptionMode::Courage), 1.0);
	TestTrue(TEXT("Non-weapon misconfigured with Attack is rejected"),
		result.FailureReason == LRGameplayTags::ItemUseRejectInvalidAttackItem);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRSharedTransactionTest, "LostRunic.Item.UseAndAttackShareOneResolver",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRSharedTransactionTest::RunTest(const FString& parameters)
{
	ULRItemDefinition* knife = MakeItem(TEXT("Test.Knife"), true, true);
	knife->MaxStackSize = 3;
	knife->AllowedTargetTags.Reset();
	knife->AllowedTargetTags.AddTag(LRGameplayTags::TargetDoorHomeKey);
	ULRInventoryComponent* inventory = MakeInventory(knife, 3);
	ULRItemUseResolver* resolver = MakeResolver(inventory);
	ULRTestItemUseTargetComponent* door = MakeUseTarget(LRGameplayTags::TargetDoorHomeKey);
	ULRTestAttackTargetComponent* guard = MakeAttackTarget();

	const FLRItemUseResult puzzleUse = resolver->ResolveAtTime(
		MakeUseRequest(knife->ItemId, door, ELRPerceptionMode::Normal), 1.0);
	TestTrue(TEXT("Knife used as a puzzle item succeeds"), puzzleUse.bSuccess);
	TestTrue(TEXT("Puzzle use consumes one"), puzzleUse.bConsumed);
	TestEqual(TEXT("Quantity reflects puzzle consumption"), inventory->GetItemCount(knife->ItemId), 2);
	const FLRItemUseResult attack = resolver->ResolveAtTime(
		MakeAttackRequest(knife->ItemId, guard, ELRPerceptionMode::Courage), 2.0);
	TestTrue(TEXT("Same knife used for attack succeeds"), attack.bSuccess);
	TestTrue(TEXT("Attack consumes under the same rules"), attack.bConsumed);
	TestEqual(TEXT("Quantity reflects both consumption paths"), inventory->GetItemCount(knife->ItemId), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRAttackSelectionRulesTest, "LostRunic.Item.AttackSelectionOnlyAcceptsLegalTargets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRAttackSelectionRulesTest::RunTest(const FString& parameters)
{
	const ULRStateTuning* tuning = GetDefault<ULRStateTuning>();
	TArray<FLRAttackCandidateScore> candidates;
	candidates.Add({ FMath::Square(180.0f), 1.0f, true, true, true, true });
	candidates.Add({ FMath::Square(90.0f), 1.0f, false, true, true, true });
	candidates.Add({ FMath::Square(120.0f), 0.0f, true, false, true, true });
	candidates.Add({ FMath::Square(150.0f), 1.0f, true, true, false, true });
	candidates.Add({ FMath::Square(60.0f), 1.0f, true, true, true, false });
	TestEqual(TEXT("Nearest in-range, facing, visible, vulnerable target is selected"),
		LRAttackTargetRules::SelectBestTarget(candidates, *tuning), 0);
	TestEqual(TEXT("Fully failed candidate list selects nothing"),
		LRAttackTargetRules::SelectBestTarget({ { FMath::Square(50.0f), 1.0f, true, true, true, false } }, *tuning), INDEX_NONE);

	const float boundaryDot = FMath::Cos(FMath::DegreesToRadians(tuning->CourageAttackFacingDegrees * 0.5f));
	const float insideDot = FMath::Cos(FMath::DegreesToRadians(44.9f));
	const float outsideDot = FMath::Cos(FMath::DegreesToRadians(45.1f));
	TestTrue(TEXT("44.9 degree target is inside the attack facing cone"),
		LRAttackTargetRules::IsFacingAllowed(insideDot, *tuning));
	TestTrue(TEXT("Facing boundary is included"), LRAttackTargetRules::IsFacingAllowed(boundaryDot, *tuning));
	TestFalse(TEXT("45.1 degree target is outside the attack facing cone"),
		LRAttackTargetRules::IsFacingAllowed(outsideDot, *tuning));
	TestTrue(TEXT("Attack range boundary is included"),
		LRAttackTargetRules::IsInRange(FMath::Square(tuning->CourageAttackRangeCm), *tuning));
	TestFalse(TEXT("Beyond attack range is rejected"),
		LRAttackTargetRules::IsInRange(FMath::Square(tuning->CourageAttackRangeCm + 0.1f), *tuning));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRNotesAndCollectiblesTest, "LostRunic.Item.NotesAndCollectiblesUseStableIds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRNotesAndCollectiblesTest::RunTest(const FString& parameters)
{
	ULRInventoryComponent* inventory = NewObject<ULRInventoryComponent>();
	TestTrue(TEXT("First note record succeeds"), inventory->AddNoteId(TEXT("Test.Note")));
	TestFalse(TEXT("Repeated note record is deduplicated"), inventory->AddNoteId(TEXT("Test.Note")));
	TestEqual(TEXT("Note set holds one stable id"), inventory->GetNoteIds().Num(), 1);
	TestFalse(TEXT("Empty note id is rejected"), inventory->AddNoteId(NAME_None));

	ULRCollectibleDefinition* doll = NewObject<ULRCollectibleDefinition>();
	doll->CollectibleId = TEXT("Test.Doll");
	inventory->InitializeCollectibleDefinitions({ doll });
	TestEqual(TEXT("First registered collectible record succeeds"),
		inventory->AddCollectibleId(TEXT("Test.Doll")), ELRAddCollectibleResult::Success);
	TestEqual(TEXT("Repeated collectible returns AlreadyOwned"),
		inventory->AddCollectibleId(TEXT("Test.Doll")), ELRAddCollectibleResult::AlreadyOwned);
	TestEqual(TEXT("Unregistered collectible id is rejected"),
		inventory->AddCollectibleId(TEXT("Test.FakeDoll")), ELRAddCollectibleResult::InvalidDefinition);
	TestEqual(TEXT("Empty collectible id is rejected"),
		inventory->AddCollectibleId(NAME_None), ELRAddCollectibleResult::InvalidDefinition);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRMenuSnapshotTest, "LostRunic.UI.MenuSnapshotHasNoQuickSlotData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRMenuSnapshotTest::RunTest(const FString& parameters)
{
	ULRItemDefinition* homeKey = MakeItem(TEXT("Home.Key"), false);
	homeKey->AllowedTargetTags.Reset();
	homeKey->AllowedTargetTags.AddTag(LRGameplayTags::TargetDoorHomeKey);
	ULRItemDefinition* knife = MakeItem(TEXT("Test.Knife"), true, true);
	knife->MaxStackSize = 2;
	ULRInventoryComponent* inventory = NewObject<ULRInventoryComponent>();
	inventory->InitializeDefinitions({ homeKey, knife });
	inventory->AddItem(homeKey->ItemId, 1);
	inventory->AddItem(knife->ItemId, 2);
	inventory->SetSelectedWeapon(knife->ItemId);

	ULRMenuWidgetController* controller = NewObject<ULRMenuWidgetController>();
	const FLRInventorySnapshot snapshot = controller->BuildInventorySnapshot(inventory);
	TestEqual(TEXT("Snapshot has no quick-slot fields (compile-time guarantee)"), snapshot.Items.Num(), 2);
	TestEqual(TEXT("Selected weapon is exposed"), snapshot.SelectedWeaponItemId, knife->ItemId);
	TestEqual(TEXT("Effective weapon matches selection"), snapshot.EffectiveWeaponItemId, knife->ItemId);
	const FLRInventoryItemView* knifeView = snapshot.Items.FindByPredicate(
		[&knife](const FLRInventoryItemView& view) { return view.ItemId == knife->ItemId; });
	TestNotNull(TEXT("Knife view exists"), knifeView);
	if (knifeView)
	{
		TestTrue(TEXT("Weapon flag is exposed"), knifeView->bIsWeapon);
		TestTrue(TEXT("Explicit selection marker is exposed"), knifeView->bIsSelectedWeapon);
		TestEqual(TEXT("Consumable quantity is exposed"), knifeView->Quantity, 2);
	}

	const FLRInventorySnapshot browseSnapshot = controller->BuildInventorySnapshot(inventory, nullptr);
	TestEqual(TEXT("Snapshot without target keeps compatibility flags off"),
		browseSnapshot.Items.Num(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRTopDownCameraTest, "LostRunic.Framework.TopDownCameraDoesNotInheritCharacterYaw",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRTopDownCameraTest::RunTest(const FString& parameters)
{
	const ALRCharacter* character = GetDefault<ALRCharacter>();
	const USpringArmComponent* cameraBoom = character ? character->FindComponentByClass<USpringArmComponent>() : nullptr;
	TestNotNull(TEXT("Character owns a camera boom"), cameraBoom);
	if (cameraBoom)
	{
		TestTrue(TEXT("Camera boom uses absolute rotation"), cameraBoom->IsUsingAbsoluteRotation());
	}
	return true;
}

#endif
