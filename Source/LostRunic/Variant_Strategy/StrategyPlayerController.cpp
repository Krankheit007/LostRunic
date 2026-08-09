// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @file StrategyPlayerController.cpp
 * @brief 保留 Unreal Strategy 模板玩法，用于回归和 PIE 冒烟；它与 /Game/LostRunic 的“家”切片相互独立，不承载 LostRunic 核心叙事规则。
 *
 * 关联文件：StrategyPlayerController.h；所属领域：Variant_Strategy。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */


#include "StrategyPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "InputMappingContext.h"
#include "Camera/CameraComponent.h"
#include "StrategyPawn.h"
#include "Camera/CameraComponent.h"
#include "InputActionValue.h"
#include "StrategyHUD.h"
#include "Engine/CollisionProfile.h"
#include "Kismet/GameplayStatics.h"
#include "StrategyUnit.h"
#include "NavigationSystem.h"
#include "Engine/OverlapResult.h"
#include "InputAction.h"
#include "StrategyTouchControls.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "LostRunic.h"

/**
 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
 */
AStrategyPlayerController::AStrategyPlayerController()
{
	// mouse cursor should always be shown
	bShowMouseCursor = true;
}

/**
 * @brief 在进入世界后解析运行时依赖、绑定事件并启动所需计时器；构造阶段不访问 World 或玩家对象。
 */
void AStrategyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController() && ShouldUseTouchControls())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UStrategyTouchControls>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

			// set the PC pointer on the mobile controls widget
			MobileControlsWidget->SetPlayerController(this);

		} else {

			UE_LOG(LogLostRunic, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}

}

/**
 * @brief 绑定 PlayerController 使用的 Enhanced Input Action；具体按键仍由 Input Mapping Context 资产决定。
 */
void AStrategyPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only set up input on local player controllers
	if (IsLocalPlayerController())
	{
		// add the input mapping context
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			// choose the context based on the input mode
			UInputMappingContext* ChosenContext = nullptr;

			if (ShouldUseTouchControls())
			{
				ChosenContext = TouchMappingContext;
			}
			else
			{
				ChosenContext = MouseMappingContext;
			}

			Subsystem->AddMappingContext(ChosenContext, 0);
		}

		// bind the input mappings
		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
		{
			// Camera
			EnhancedInputComponent->BindAction(MoveCameraAction, ETriggerEvent::Triggered, this, &AStrategyPlayerController::MoveCamera);
			EnhancedInputComponent->BindAction(ZoomCameraAction, ETriggerEvent::Triggered, this, &AStrategyPlayerController::ZoomCamera);
			EnhancedInputComponent->BindAction(ResetCameraAction, ETriggerEvent::Triggered, this, &AStrategyPlayerController::ResetCamera);

			// Mouse Interaction
			EnhancedInputComponent->BindAction(SelectHoldAction, ETriggerEvent::Started, this, &AStrategyPlayerController::SelectHoldStarted);
			EnhancedInputComponent->BindAction(SelectHoldAction, ETriggerEvent::Triggered, this, &AStrategyPlayerController::SelectHoldTriggered);
			EnhancedInputComponent->BindAction(SelectHoldAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::SelectHoldCompleted);
			EnhancedInputComponent->BindAction(SelectHoldAction, ETriggerEvent::Canceled, this, &AStrategyPlayerController::SelectHoldCompleted);

			EnhancedInputComponent->BindAction(SelectClickAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::SelectClick);

			EnhancedInputComponent->BindAction(SelectClickAdditiveAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::SelectClickAdditive);

			EnhancedInputComponent->BindAction(SelectAllDoubleClickAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::SelectAllDoubleClick);

			EnhancedInputComponent->BindAction(InteractHoldAction, ETriggerEvent::Started, this, &AStrategyPlayerController::InteractHoldStarted);
			EnhancedInputComponent->BindAction(InteractHoldAction, ETriggerEvent::Triggered, this, &AStrategyPlayerController::InteractHoldTriggered);

			EnhancedInputComponent->BindAction(InteractClickAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::InteractClick);

			// Touch Interaction
			EnhancedInputComponent->BindAction(TouchPrimaryHoldAction, ETriggerEvent::Started, this, &AStrategyPlayerController::TouchPrimaryHoldStarted);
			EnhancedInputComponent->BindAction(TouchPrimaryHoldAction, ETriggerEvent::Triggered, this, &AStrategyPlayerController::TouchPrimaryHoldTriggered);
			EnhancedInputComponent->BindAction(TouchPrimaryHoldAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::TouchPrimaryHoldCompleted);

			EnhancedInputComponent->BindAction(TouchSecondaryAction, ETriggerEvent::Triggered, this, &AStrategyPlayerController::TouchSecondaryTriggered);
			EnhancedInputComponent->BindAction(TouchSecondaryAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::TouchSecondaryCompleted);
			EnhancedInputComponent->BindAction(TouchSecondaryAction, ETriggerEvent::Canceled, this, &AStrategyPlayerController::TouchSecondaryCompleted);

		}
	}
}

/**
 * @brief 处理 On Possess 事件，将引擎回调转换为对应领域状态更新。
 * @param InPawn Controller 新接管的 Pawn；期望为 ALRGuardCharacter。
 */
void AStrategyPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// ensure we have the right pawn type
	ControlledCameraPawn = Cast<AStrategyPawn>(InPawn);
	check(ControlledCameraPawn);

	// set the zoom level from the pawn's camera
	DefaultZoom = CameraZoom = ControlledCameraPawn->GetCamera()->OrthoWidth;

	// cast the HUD pointer
	StrategyHUD = Cast<AStrategyHUD>(GetHUD());

	// if we have a touch controls widget, sync the camera zoom
	if (MobileControlsWidget)
	{
		MobileControlsWidget->BP_SetZoomPercentage(GetDefaultZoomPercentage());
	}
}

/**
 * @brief 实现 Drag Select Units 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 * @param Units 调用方提供的 `Units`，只在本次操作范围内使用。
 */
void AStrategyPlayerController::DragSelectUnits(const TArray<AStrategyUnit*>& Units)
{
	// do we have units in the list?
	if (Units.Num() > 0)
	{
		// ensure any previous units are deselected
		DoDeselectAllUnitsCommand();

		// select each new unit
		for (AStrategyUnit* CurrentUnit : Units)
		{
			// add the unit to the selection list
			ControlledUnits.Add(CurrentUnit);

			// select the unit
			CurrentUnit->UnitSelected();
		}

	}
	else
	{

		// release any currently selected units since nothing is on the box
		if (ControlledUnits.Num() > 0)
		{
			DoDeselectAllUnitsCommand();
		}

	}
}

/**
 * @brief 查询 Selected Units；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
const TArray<AStrategyUnit*>& AStrategyPlayerController::GetSelectedUnits()
{
	return ControlledUnits;
}

/**
 * @brief 查询 Default Zoom Percentage；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
float AStrategyPlayerController::GetDefaultZoomPercentage() const
{
	float ZoomPct = (DefaultZoom - MinZoomLevel) / (MaxZoomLevel - MinZoomLevel);
	return FMath::Clamp(ZoomPct, 0.0f, 1.0f);
}

/**
 * @brief 判断 Should Use Touch Controls 对应条件；不产生玩法副作用。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool AStrategyPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

/**
 * @brief 执行 Move Camera 的玩法动作；输入层只提供语义，合法性由对应领域组件决定。
 * @param Value 本次输入、状态更新或测试使用的值。
 */
void AStrategyPlayerController::MoveCamera(const FInputActionValue& Value)
{
	FVector2D InputVector = Value.Get<FVector2D>();

	// get the forward input component vector
	FRotator ForwardRot = GetControlRotation();
	ForwardRot.Pitch = 0.0f;

	// get the right input component vector
	FRotator RightRot = GetControlRotation();
	RightRot.Pitch = 0.0f;
	RightRot.Roll = 0.0f;

	// add the forward input
	if (ControlledCameraPawn)
	{
		ControlledCameraPawn->AddMovementInput(ForwardRot.RotateVector(FVector::ForwardVector), InputVector.X + InputVector.Y);

		// add the right input
		ControlledCameraPawn->AddMovementInput(RightRot.RotateVector(FVector::RightVector), InputVector.X - InputVector.Y);
	}
}

/**
 * @brief 实现 Zoom Camera 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 * @param Value 本次输入、状态更新或测试使用的值。
 */
void AStrategyPlayerController::ZoomCamera(const FInputActionValue& Value)
{
	DoCameraModifyZoomCommand(Value.Get<float>() * ZoomScaling);
}

/**
 * @brief 实现 Reset Camera 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 * @param Value 本次输入、状态更新或测试使用的值。
 */
void AStrategyPlayerController::ResetCamera(const FInputActionValue& Value)
{
	DoCameraResetZoomCommand();
}

/**
 * @brief 执行 Select Hold Started 的纯规则或事务判定，失败时提供结构化原因。
 * @param Value 本次输入、状态更新或测试使用的值。
 */
void AStrategyPlayerController::SelectHoldStarted(const FInputActionValue& Value)
{
	// save the box selection start position
	StartingBoxSelectionPosition = GetMouseLocationForPlayer();

}

/**
 * @brief 执行 Select Hold Triggered 的纯规则或事务判定，失败时提供结构化原因。
 * @param Value 本次输入、状态更新或测试使用的值。
 */
void AStrategyPlayerController::SelectHoldTriggered(const FInputActionValue& Value)
{
	// get the current mouse position
	FVector2D SelectionPosition = GetMouseLocationForPlayer();

	// calculate the size of the selection box
	FVector2D SelectionSize = SelectionPosition - StartingBoxSelectionPosition;

	// update the selection box on the HUD
	if (StrategyHUD)
	{
		StrategyHUD->DragSelectUpdate(StartingBoxSelectionPosition, SelectionSize, SelectionPosition, true);
	}
}

/**
 * @brief 执行 Select Hold Completed 的纯规则或事务判定，失败时提供结构化原因。
 * @param Value 本次输入、状态更新或测试使用的值。
 */
void AStrategyPlayerController::SelectHoldCompleted(const FInputActionValue& Value)
{
	// reset the drag box on the HUD
	if (StrategyHUD)
	{
		StrategyHUD->DragSelectUpdate(FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector, false);
	}
}

/**
 * @brief 执行 Select Click 的纯规则或事务判定，失败时提供结构化原因。
 * @param Value 本次输入、状态更新或测试使用的值。
 */
void AStrategyPlayerController::SelectClick(const FInputActionValue& Value)
{
	// get the cursor location
	FVector CursorLocation;

	if (GetLocationUnderCursor(CursorLocation))
	{
		// select at the cursor
		DoSelectCommand(CursorLocation, false);
	}
}

/**
 * @brief 执行 Select Click Additive 的纯规则或事务判定，失败时提供结构化原因。
 * @param Value 本次输入、状态更新或测试使用的值。
 */
void AStrategyPlayerController::SelectClickAdditive(const FInputActionValue& Value)
{
	// get the cursor location
	FVector CursorLocation;

	if (GetLocationUnderCursor(CursorLocation))
	{
		// additive select at the cursor
		DoSelectCommand(CursorLocation, true);
	}
}

/**
 * @brief 执行 Select All Double Click 的纯规则或事务判定，失败时提供结构化原因。
 * @param Value 本次输入、状态更新或测试使用的值。
 */
void AStrategyPlayerController::SelectAllDoubleClick(const FInputActionValue& Value)
{
	DoSelectAllUnitsOnScreenCommand();
}

/**
 * @brief 执行 Interact Hold Started 的玩法动作；输入层只提供语义，合法性由对应领域组件决定。
 * @param Value 本次输入、状态更新或测试使用的值。
 */
void AStrategyPlayerController::InteractHoldStarted(const FInputActionValue& Value)
{

	// save the starting interaction position
	StartingDragScrollPosition = GetMouseLocationForPlayer();
}

/**
 * @brief 执行 Interact Hold Triggered 的玩法动作；输入层只提供语义，合法性由对应领域组件决定。
 * @param Value 本次输入、状态更新或测试使用的值。
 */
void AStrategyPlayerController::InteractHoldTriggered(const FInputActionValue& Value)
{
	// do a drag scroll
	DoCameraDragScrollCommand(GetMouseLocationForPlayer());
}

/**
 * @brief 执行 Interact Click 的玩法动作；输入层只提供语义，合法性由对应领域组件决定。
 * @param Value 本次输入、状态更新或测试使用的值。
 */
void AStrategyPlayerController::InteractClick(const FInputActionValue& Value)
{
	// get the cursor location
	FVector CursorLocation;

	// do we have a valid interaction location under the cursor?
	if (GetLocationUnderCursor(CursorLocation))
	{
		// move the selected units to the target location
		DoMoveUnitsCommand(CursorLocation);
	}
}

/**
 * @brief 实现 Touch Primary Hold Started 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 * @param Value 本次输入、状态更新或测试使用的值。
 */
void AStrategyPlayerController::TouchPrimaryHoldStarted(const FInputActionValue& Value)
{
	// save the camera drag screen coords
	StartingDragScrollPosition = Value.Get<FVector2D>();


}

/**
 * @brief 实现 Touch Primary Hold Triggered 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 * @param Instance 调用方提供的 `Instance`，只在本次操作范围内使用。
 */
void AStrategyPlayerController::TouchPrimaryHoldTriggered(const FInputActionInstance& Instance)
{
	FVector2D InputVector = Instance.GetValue().Get<FVector2D>();

	// update the box select start position
	StartingBoxSelectionPosition = InputVector;

	if (Instance.GetElapsedTime() > TouchDragScrollHoldTime)
	{
		DoCameraDragScrollCommand(InputVector);

		// save the game time
		LastTouchDragScrollTime = GetWorld()->GetTimeSeconds();
	}
}

/**
 * @brief 实现 Touch Primary Hold Completed 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 * @param Value 本次输入、状态更新或测试使用的值。
 */
void AStrategyPlayerController::TouchPrimaryHoldCompleted(const FInputActionValue& Value)
{
	// ensure we don't trigger a tap input right after we finish a drag scroll
	if (GetWorld()->GetTimeSeconds() - LastTouchDragScrollTime > 0.1f)
	{
		// get the touch location in world space
		FVector TouchLocation = ProjectTouchPointToWorldSpace();

		// try to do a select command
		if (!DoSelectCommand(TouchLocation, true))
		{
			// if nothing was selected, do a move units command instead
			DoMoveUnitsCommand(TouchLocation);
		}
	}
}

/**
 * @brief 实现 Touch Secondary Triggered 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 * @param Value 本次输入、状态更新或测试使用的值。
 */
void AStrategyPlayerController::TouchSecondaryTriggered(const FInputActionValue& Value)
{
	// get the touch 2 screen coords
	FVector2D SelectionPosition = Value.Get<FVector2D>();

	// calculate the size of the selection box
	FVector2D SelectionSize = SelectionPosition - StartingBoxSelectionPosition;

	// update the selection box on the HUD
	if (StrategyHUD)
	{
		StrategyHUD->DragSelectUpdate(StartingBoxSelectionPosition, SelectionSize, SelectionPosition, true);
	}

}

/**
 * @brief 实现 Touch Secondary Completed 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 * @param Value 本次输入、状态更新或测试使用的值。
 */
void AStrategyPlayerController::TouchSecondaryCompleted(const FInputActionValue& Value)
{
	if (StrategyHUD)
	{
		// hide the selection box
		StrategyHUD->DragSelectUpdate(FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector, false);
	}
}

/**
 * @brief 实现 Do Select Command 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 * @param SelectLocation 空间值 `SelectLocation`；距离和位置使用 Unreal 厘米单位。
 * @param bAdditiveSelection 布尔开关 `bAdditiveSelection`；true 表示启用或条件成立，false 表示禁用或条件不成立。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool AStrategyPlayerController::DoSelectCommand(const FVector& SelectLocation, bool bAdditiveSelection)
{
	// deselect any units unless this is an additive selection
	if (!bAdditiveSelection)
	{
		DoDeselectAllUnitsCommand();
	}

	// do an overlap test at the cursor location
	TArray<FOverlapResult> OutOverlaps;

	FCollisionShape CollisionSphere;
	CollisionSphere.SetSphere(SelectionRadius);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams;

	if (GetWorld()->OverlapMultiByObjectType(OutOverlaps, SelectLocation, FQuat::Identity, ObjectParams, CollisionSphere, QueryParams))
	{
		// find the first unit we've overlapped
		for (const FOverlapResult& CurrentOverlap : OutOverlaps)
		{
			if (AStrategyUnit* CurrentUnit = Cast<AStrategyUnit>(CurrentOverlap.GetActor()))
			{
				// is this unit already selected?
				if (ControlledUnits.Contains(CurrentUnit))
				{
					// deselect the unit
					ControlledUnits.Remove(CurrentUnit);

					CurrentUnit->UnitDeselected();
				}
				else
				{
					// select the unit
					ControlledUnits.Add(CurrentUnit);

					CurrentUnit->UnitSelected();
				}

				// found a unit
				return true;
			}
		}
	}

	// didn't find a unit
	return false;
}

/**
 * @brief 实现 Do Select All Units On Screen Command 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 */
void AStrategyPlayerController::DoSelectAllUnitsOnScreenCommand()
{
	// get all units on the level
	TArray<AActor*> Units;

	UGameplayStatics::GetAllActorsOfClass(this, AStrategyUnit::StaticClass(), Units);

	// process each unit
	for (AActor* CurrentActor : Units)
	{
		if (AStrategyUnit* CurrentUnit = Cast<AStrategyUnit>(CurrentActor))
		{
			// is the unit is not already selected, and is on screen?
			if (!ControlledUnits.Contains(CurrentUnit) && CurrentUnit->WasRecentlyRendered(0.2f))
			{
				// select the unit
				ControlledUnits.Add(CurrentUnit);

				CurrentUnit->UnitSelected();
			}
		}

	}
}

/**
 * @brief 实现 Do Deselect All Units Command 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 */
void AStrategyPlayerController::DoDeselectAllUnitsCommand()
{
	// deselect each unit
	for (AStrategyUnit* CurrentUnit : ControlledUnits)
	{
		if (IsValid(CurrentUnit))
		{
			CurrentUnit->UnitDeselected();
		}
	}

	// clear the selection list
	ControlledUnits.Empty();
}

/**
 * @brief 实现 Do Toggle Select All Units Command 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 */
void AStrategyPlayerController::DoToggleSelectAllUnitsCommand()
{
	// do we have units selected?
	if (ControlledUnits.Num() > 0)
	{
		// deselect all units
		DoDeselectAllUnitsCommand();
	}
	else
	{
		// select all units on screen
		DoSelectAllUnitsOnScreenCommand();
	}
}

/**
 * @brief 实现 Do Camera Drag Scroll Command 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 * @param CurrentCursorPosition 调用方提供的 `CurrentCursorPosition`，只在本次操作范围内使用。
 */
void AStrategyPlayerController::DoCameraDragScrollCommand(const FVector2D& CurrentCursorPosition)
{
	// subtract the starting position from the cursor to find the on-screen movement delta
	FVector2D MoveDelta = StartingDragScrollPosition - CurrentCursorPosition;

	// rotate the movement delta to match the isometric perspective
	const FRotator IsoRotation(0.0f, -45.0f, 0.0f);

	FVector RotatedDelta = IsoRotation.RotateVector(FVector(MoveDelta.X, MoveDelta.Y, 0.0f));

	// apply drag
	RotatedDelta *= DragMultiplier;

	// apply the offset to the camera pawn
	if (ControlledCameraPawn)
	{
		ControlledCameraPawn->AddActorWorldOffset(RotatedDelta);
	}
}

/**
 * @brief 实现 Do Move Units Command 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 * @param GoalLocation 空间值 `GoalLocation`；距离和位置使用 Unreal 厘米单位。
 */
void AStrategyPlayerController::DoMoveUnitsCommand(const FVector& GoalLocation)
{
	if (ControlledUnits.Num() > 0)
	{
		// find the closest unit to the goal
		AStrategyUnit* ClosestUnit = GetClosestSelectedUnitToLocation(GoalLocation);

		// tell each unit to move to the location
		for (AStrategyUnit* CurrentUnit : ControlledUnits)
		{
			if (IsValid(CurrentUnit))
			{
				CurrentUnit->MoveToLocation(GoalLocation, CurrentUnit == ClosestUnit, ControlledUnits);
			}
		}

		// show positive cursor feedback
		BP_CursorFeedback(GoalLocation, true);

	}
	else
	{
		// no units selected, so just show negative cursor feedback
		BP_CursorFeedback(GoalLocation, false);
	}
}

/**
 * @brief 实现 Do Camera Modify Zoom Command 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 * @param ZoomDelta 调用方提供的 `ZoomDelta`，只在本次操作范围内使用。
 */
void AStrategyPlayerController::DoCameraModifyZoomCommand(float ZoomDelta)
{
	// add the delta
	CameraZoom += ZoomDelta;

	// clamp between min and max
	CameraZoom = FMath::Clamp(CameraZoom, MinZoomLevel, MaxZoomLevel);

	// set the zoom on the camera pawn
	if (ControlledCameraPawn)
	{
		ControlledCameraPawn->SetZoomModifier(CameraZoom);
	}
}

/**
 * @brief 实现 Do Camera Reset Zoom Command 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 */
void AStrategyPlayerController::DoCameraResetZoomCommand()
{
	// reset to default zoom
	CameraZoom = DefaultZoom;

	// set the zoom on the camera pawn
	if (ControlledCameraPawn)
	{
		ControlledCameraPawn->SetZoomModifier(CameraZoom);
	}
}

/**
 * @brief 实现 Do Camera Set Zoom Percentage Command 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 * @param Percentage 调用方提供的 `Percentage`，只在本次操作范围内使用。
 */
void AStrategyPlayerController::DoCameraSetZoomPercentageCommand(float Percentage)
{
	// lerp between min and max zoom
	CameraZoom = FMath::Lerp(MinZoomLevel, MaxZoomLevel, FMath::Clamp(Percentage, 0.0f, 1.0f));

	// set the zoom on the camera pawn
	if (ControlledCameraPawn)
	{
		ControlledCameraPawn->SetZoomModifier(CameraZoom);
	}
}

/**
 * @brief 查询 Closest Selected Unit To Location；不修改领域状态。
 * @param TargetLocation 空间值 `TargetLocation`；距离和位置使用 Unreal 厘米单位。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
AStrategyUnit* AStrategyPlayerController::GetClosestSelectedUnitToLocation(FVector TargetLocation)
{
	// closest unit and distance
	AStrategyUnit* OutUnit = nullptr;
	float Closest = 0.0f;

	// process each unit on the list
	for (AStrategyUnit* CurrentUnit : ControlledUnits)
	{
		if (IsValid(CurrentUnit))
		{
			// have we selected a unit already?
			if (OutUnit != nullptr)
			{
				// calculate the squared distance to the target location
				float Dist = FVector::DistSquared2D(TargetLocation, CurrentUnit->GetActorLocation());

				// is this unit closer?
				if (Dist < Closest)
				{
					// update the closest unit and distance
					OutUnit = CurrentUnit;
					Closest = Dist;
				}

			}
			else
			{

				// no previously selected unit, so use this one
				OutUnit = CurrentUnit;

				// initialize the closest distance
				Closest = FVector::DistSquared2D(TargetLocation, CurrentUnit->GetActorLocation());
			}
		}

	}

	// return the selected unit
	return OutUnit;
}

/**
 * @brief 查询 Mouse Location For Player；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FVector2D AStrategyPlayerController::GetMouseLocationForPlayer()
{
	// attempt to get the mouse position from this PC
	float MouseX, MouseY;

	if (GetMousePosition(MouseX, MouseY))
	{
		return FVector2D(MouseX, MouseY);
	}

	// return an invalid vector
	return FVector2D::ZeroVector;
}

/**
 * @brief 查询 Location Under Cursor；不修改领域状态。
 * @param Location 世界空间位置，Unreal 单位为厘米。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool AStrategyPlayerController::GetLocationUnderCursor(FVector& Location)
{
	// trace the visibility channel at the cursor location
	FHitResult OutHit;

	GetHitResultUnderCursorByChannel(SelectionTraceChannel, false, OutHit);

	// if there was a blocking hit, return the hit location
	if (OutHit.bBlockingHit)
	{
		Location = OutHit.Location;
		return true;
	}

	return OutHit.bBlockingHit;
}

/**
 * @brief 查询 Location Under Finger；不修改领域状态。
 * @param Location 世界空间位置，Unreal 单位为厘米。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool AStrategyPlayerController::GetLocationUnderFinger(FVector& Location)
{
	// trace the visibility channel at Touch 1 location
	FHitResult OutHit;

	GetHitResultUnderFingerByChannel(ETouchIndex::Touch1, SelectionTraceChannel, false, OutHit);

	// if there was a blocking hit, return the hit location
	if (OutHit.bBlockingHit)
	{
		Location = OutHit.Location;
		return true;
	}

	return OutHit.bBlockingHit;
}

/**
 * @brief 实现 Project Touch Point To World Space 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FVector AStrategyPlayerController::ProjectTouchPointToWorldSpace()
{
	// get the touch coordinates for the first finger
	float TouchX, TouchY = 0.0f;
	bool bPressed = false;

	GetInputTouchState(ETouchIndex::Touch1, TouchX, TouchY, bPressed);

	FVector WorldLocation = FVector::ZeroVector;
	FVector WorldDirection = FVector::ZeroVector;

	// deproject the coords into world space
	if (DeprojectScreenPositionToWorld(TouchX, TouchY, WorldLocation, WorldDirection))
	{
		// run a line trace down the camera
		FHitResult OutHit;

		GetWorld()->LineTraceSingleByChannel(OutHit, WorldLocation, WorldLocation + WorldDirection * 10000.0f, ECC_Visibility);

		// if we hit something, return the impact point
		if (OutHit.bBlockingHit)
		{
			return OutHit.ImpactPoint;
		}


		// intersect with a horizontal plane and return the resulting point
		const FPlane IntersectPlane(FVector::ZeroVector, FVector::UpVector);
		return FMath::LinePlaneIntersection(WorldLocation, WorldLocation + (WorldDirection * 100000.0f), IntersectPlane);
	}

	// failed to deproject, return a zero vector
	return FVector::ZeroVector;
}
