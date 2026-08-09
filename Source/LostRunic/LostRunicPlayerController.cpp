// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @file LostRunicPlayerController.cpp
 * @brief 提供 Unreal 模块入口及原始 TopDown 模板兼容类；新的“家”垂直切片使用 Framework、State、Interaction、AI、Narrative 与 Save 目录中的 LR 领域实现。
 *
 * 关联文件：LostRunicPlayerController.h；所属领域：Root。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */

#include "LostRunicPlayerController.h"
#include "GameFramework/Pawn.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "LostRunicCharacter.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "InputActionValue.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "LostRunic.h"

/**
 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
 */
ALostRunicPlayerController::ALostRunicPlayerController()
{
	bIsTouch = false;
	bMoveToMouseCursor = false;

	// create the path following comp
	PathFollowingComponent = CreateDefaultSubobject<UPathFollowingComponent>(TEXT("Path Following Component"));

	// configure the controller
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
	CachedDestination = FVector::ZeroVector;
	FollowTime = 0.f;
}

/**
 * @brief 绑定 PlayerController 使用的 Enhanced Input Action；具体按键仍由 Input Mapping Context 资产决定。
 */
void ALostRunicPlayerController::SetupInputComponent()
{
	// set up gameplay key bindings
	Super::SetupInputComponent();

	// Only set up input on local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Context
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}

		// Set up action bindings
		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
		{
			// Setup mouse input events
			EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Started, this, &ALostRunicPlayerController::OnInputStarted);
			EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Triggered, this, &ALostRunicPlayerController::OnSetDestinationTriggered);
			EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Completed, this, &ALostRunicPlayerController::OnSetDestinationReleased);
			EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Canceled, this, &ALostRunicPlayerController::OnSetDestinationReleased);

			// Setup touch input events
			EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Started, this, &ALostRunicPlayerController::OnInputStarted);
			EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Triggered, this, &ALostRunicPlayerController::OnTouchTriggered);
			EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Completed, this, &ALostRunicPlayerController::OnTouchReleased);
			EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Canceled, this, &ALostRunicPlayerController::OnTouchReleased);
		}
		else
		{
			UE_LOG(LogLostRunic, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
		}
	}
}

/**
 * @brief 处理 On Input Started 事件，将引擎回调转换为对应领域状态更新。
 */
void ALostRunicPlayerController::OnInputStarted()
{
	StopMovement();

	// Update the move destination to wherever the cursor is pointing at
	UpdateCachedDestination();
}

/**
 * @brief 处理 On Set Destination Triggered 事件，将引擎回调转换为对应领域状态更新。
 */
void ALostRunicPlayerController::OnSetDestinationTriggered()
{
	// We flag that the input is being pressed
	FollowTime += GetWorld()->GetDeltaSeconds();

	// Update the move destination to wherever the cursor is pointing at
	UpdateCachedDestination();

	// Move towards mouse pointer or touch
	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn != nullptr)
	{
		FVector WorldDirection = (CachedDestination - ControlledPawn->GetActorLocation()).GetSafeNormal();
		ControlledPawn->AddMovementInput(WorldDirection, 1.0, false);
	}
}

/**
 * @brief 处理 On Set Destination Released 事件，将引擎回调转换为对应领域状态更新。
 */
void ALostRunicPlayerController::OnSetDestinationReleased()
{
	// If it was a short press
	if (FollowTime <= ShortPressThreshold)
	{
		// We move there and spawn some particles
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, CachedDestination);
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, FXCursor, CachedDestination, FRotator::ZeroRotator, FVector(1.f, 1.f, 1.f), true, true, ENCPoolMethod::None, true);
	}

	FollowTime = 0.f;
}

// Triggered every frame when the input is held down
/**
 * @brief 处理 On Touch Triggered 事件，将引擎回调转换为对应领域状态更新。
 */
void ALostRunicPlayerController::OnTouchTriggered()
{
	bIsTouch = true;
	OnSetDestinationTriggered();
}

/**
 * @brief 处理 On Touch Released 事件，将引擎回调转换为对应领域状态更新。
 */
void ALostRunicPlayerController::OnTouchReleased()
{
	bIsTouch = false;
	OnSetDestinationReleased();
}

/**
 * @brief 根据最新领域状态刷新 Update Cached Destination，并仅在值变化时通知订阅者。
 */
void ALostRunicPlayerController::UpdateCachedDestination()
{
	// We look for the location in the world where the player has pressed the input
	FHitResult Hit;
	bool bHitSuccessful = false;
	if (bIsTouch)
	{
		bHitSuccessful = GetHitResultUnderFinger(ETouchIndex::Touch1, ECollisionChannel::ECC_Visibility, true, Hit);
	}
	else
	{
		bHitSuccessful = GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, true, Hit);
	}

	// If we hit a surface, cache the location
	if (bHitSuccessful)
	{
		CachedDestination = Hit.Location;
	}
}
