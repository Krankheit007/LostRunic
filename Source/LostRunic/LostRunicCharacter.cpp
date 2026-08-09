// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @file LostRunicCharacter.cpp
 * @brief 提供 Unreal 模块入口及原始 TopDown 模板兼容类；新的“家”垂直切片使用 Framework、State、Interaction、AI、Narrative 与 Save 目录中的 LR 领域实现。
 *
 * 关联文件：LostRunicCharacter.h；所属领域：Root。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */

#include "LostRunicCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/Material.h"
#include "Engine/World.h"

/**
 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
 */
ALostRunicCharacter::ALostRunicCharacter()
{
	// Set size for player capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate character to camera direction
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	// Create the camera boom component
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));

	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false;

	// Create the camera component
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));

	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false;

	// Activate ticking in order to update the cursor every frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

/**
 * @brief 在进入世界后解析运行时依赖、绑定事件并启动所需计时器；构造阶段不访问 World 或玩家对象。
 */
void ALostRunicCharacter::BeginPlay()
{
	Super::BeginPlay();

	// stub
}

/**
 * @brief 实现 Tick 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 * @param DeltaSeconds 自上次更新经过的秒数，仅用于显式的时间累计。
 */
void ALostRunicCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

	// stub
}
