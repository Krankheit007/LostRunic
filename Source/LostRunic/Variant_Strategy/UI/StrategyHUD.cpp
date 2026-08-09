// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @file StrategyHUD.cpp
 * @brief 保留 Unreal Strategy 模板玩法，用于回归和 PIE 冒烟；它与 /Game/LostRunic 的“家”切片相互独立，不承载 LostRunic 核心叙事规则。
 *
 * 关联文件：StrategyHUD.h；所属领域：Variant_Strategy。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */


#include "StrategyHUD.h"
#include "StrategyUnit.h"
#include "StrategyPlayerController.h"
#include "StrategyUI.h"

/**
 * @brief 在进入世界后解析运行时依赖、绑定事件并启动所需计时器；构造阶段不访问 World 或玩家对象。
 */
void AStrategyHUD::BeginPlay()
{
	Super::BeginPlay();

	// spawn the UI widget
	UIWidget = CreateWidget<UStrategyUI>(GetOwningPlayerController(), UIWidgetClass);
	check(UIWidget);

	// add the UI widget to the screen
	UIWidget->AddToViewport(0);
}

/**
 * @brief 实现 Drag Select Update 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 * @param Start 调用方提供的 `Start`，只在本次操作范围内使用。
 * @param WidthAndHeight 调用方提供的 `WidthAndHeight`，只在本次操作范围内使用。
 * @param CurrentPosition 调用方提供的 `CurrentPosition`，只在本次操作范围内使用。
 * @param bDraw 布尔开关 `bDraw`；true 表示启用或条件成立，false 表示禁用或条件不成立。
 */
void AStrategyHUD::DragSelectUpdate(FVector2D Start, FVector2D WidthAndHeight, FVector2D CurrentPosition, bool bDraw)
{
	// copy the selection box data
	bDrawBox = bDraw;
	BoxStart = Start;
	BoxSize = WidthAndHeight;
	BoxCurrentPosition = CurrentPosition;

}

/**
 * @brief 实现 Draw HUD 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 */
void AStrategyHUD::DrawHUD()
{
	// draw all debug information, etc.
	Super::DrawHUD();

	// ensure we have a valid player controller
	if (AStrategyPlayerController* PC = Cast<AStrategyPlayerController>(GetOwningPlayerController()))
	{
		// draw the selection box
		if (bDrawBox)
		{
			DrawRect(SelectionBoxColor, BoxStart.X, BoxStart.Y, BoxSize.X, BoxSize.Y);

			// get all the units in the selection box
			TArray<AStrategyUnit*> BoxedUnits;
			GetActorsInSelectionRectangle(BoxStart, BoxCurrentPosition, BoxedUnits, true);

			// update the unit selection on the player controller
			PC->DragSelectUnits(BoxedUnits);
		}

		// get the currently selected units
		TArray<AStrategyUnit*> SelectedUnits = PC->GetSelectedUnits();

		// update the selection count on the UI widget
		if (UIWidget)
		{
			UIWidget->SetSelectedUnitsCount(SelectedUnits.Num());
		}

		// process each selected unit
		for (AStrategyUnit* CurrentUnit : SelectedUnits)
		{
			if (IsValid(CurrentUnit))
			{
				// project the unit's location to screen coordinates
				FVector2D ScreenCoords;

				if (PC->ProjectWorldLocationToScreen(CurrentUnit->GetActorLocation(), ScreenCoords, true))
				{
					// draw a selection string near the unit
					const FString SelectionString = "Selected";
					DrawText(SelectionString, FColor::White, ScreenCoords.X - 25.0f, ScreenCoords.Y + 25.0f, nullptr, 1.5f);
				}
			}

		}
	}

}
