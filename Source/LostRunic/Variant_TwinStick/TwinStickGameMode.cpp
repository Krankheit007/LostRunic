// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @file TwinStickGameMode.cpp
 * @brief 保留 Unreal TwinStick 模板玩法，用于回归和 PIE 冒烟；它与 /Game/LostRunic 的“家”切片相互独立，不承载 LostRunic 核心叙事规则。
 *
 * 关联文件：TwinStickGameMode.h；所属领域：Variant_TwinStick。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */


#include "TwinStickGameMode.h"
#include "TwinStickUI.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

/**
 * @brief 在进入世界后解析运行时依赖、绑定事件并启动所需计时器；构造阶段不访问 World 或玩家对象。
 */
void ATwinStickGameMode::BeginPlay()
{
	// create the UI widget if it hasn't already
	CreateUI();
}

/**
 * @brief 解除委托并清理计时器或缓存，避免关卡切换和对象销毁后继续收到回调。
 * @param EndPlayReason Unreal 提供的结束原因，用于区分销毁、关卡切换和退出。
 */
void ATwinStickGameMode::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// clear the combo timer
	GetWorld()->GetTimerManager().ClearTimer(ComboTimer);
}

/**
 * @brief 实现 Item Used 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 * @param Value 本次输入、状态更新或测试使用的值。
 */
void ATwinStickGameMode::ItemUsed(int32 Value)
{
	// ensure the UI widget is available
	if (!UIWidget)
	{
		CreateUI();
	}

	// update the UI
	if (UIWidget)
	{
		UIWidget->UpdateItems(Value);
	}
}

/**
 * @brief 实现 Score Update 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 * @param Value 本次输入、状态更新或测试使用的值。
 */
void ATwinStickGameMode::ScoreUpdate(int32 Value)
{
	// multiply the base score by the combo multiplier and add it to the score
	Score += Value * Combo;

	// update the UI
	if (UIWidget)
	{
		UIWidget->UpdateScore(Score);
	}

	// update the combo multiplier
	ComboUpdate();
}

/**
 * @brief 根据当前领域状态构建 Create UI 所需的数据，不把临时对象作为长期存档标识。
 */
void ATwinStickGameMode::CreateUI()
{
	// avoid creating the UI multiple times
	if(UIWidget)
		return;

	// create the UI widget and add it to the viewport
	UIWidget = CreateWidget<UTwinStickUI>(UGameplayStatics::GetPlayerController(GetWorld(), 0), UIWidgetClass);

	if (UIWidget)
	{
		UIWidget->AddToViewport(0);
	}
}

/**
 * @brief 实现 Combo Update 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 */
void ATwinStickGameMode::ComboUpdate()
{
	// return
	if (Combo > ComboCap)
	{
		return;
	}

	// update the combo increment
	++ComboIncrement;

	// is it time to increase the multiplier?
	if (ComboIncrement > ComboIncrementMax)
	{
		// reset the combo increment
		ComboIncrement = 0;

		// increase the combo multiplier
		++Combo;

		// update the UI
		if (UIWidget)
		{
			UIWidget->UpdateCombo(Combo);
		}

	}

	// reset the cooldown timer
	ResetComboCooldown();
}

/**
 * @brief 实现 Reset Combo Cooldown 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 */
void ATwinStickGameMode::ResetComboCooldown()
{
	// reset the combo cooldown timer
	GetWorld()->GetTimerManager().SetTimer(ComboTimer, this, &ATwinStickGameMode::ResetCombo, ComboCooldown, false);
}

/**
 * @brief 实现 Reset Combo 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 */
void ATwinStickGameMode::ResetCombo()
{
	// is the combo multiplier above min?
	if (Combo > 1)
	{
		// reset the combo increment
		ComboIncrement = 0;

		// tick down the multiplier
		--Combo;

		// update the UI
		if (UIWidget)
		{
			UIWidget->UpdateCombo(Combo);
		}

		// reset the cooldown timer
		ResetComboCooldown();
	}
}

/**
 * @brief 判断 Can Spawn NPCs 对应条件；不产生玩法副作用。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ATwinStickGameMode::CanSpawnNPCs()
{
	// is the NPC counter under the cap?
	return NPCCount < NPCCap;
}

/**
 * @brief 实现 Increase NPCs 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 */
void ATwinStickGameMode::IncreaseNPCs()
{
	// increase the NPC counter
	++NPCCount;
}

/**
 * @brief 实现 Decrease NPCs 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 */
void ATwinStickGameMode::DecreaseNPCs()
{
	// decrease the NPC counter
	--NPCCount;
}
