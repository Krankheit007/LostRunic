// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @file TwinStickCharacter.cpp
 * @brief 保留 Unreal TwinStick 模板玩法，用于回归和 PIE 冒烟；它与 /Game/LostRunic 的“家”切片相互独立，不承载 LostRunic 核心叙事规则。
 *
 * 关联文件：TwinStickCharacter.h；所属领域：Variant_TwinStick。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */


#include "TwinStickCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "TwinStickGameMode.h"
#include "TwinStickAoEAttack.h"
#include "Kismet/KismetMathLibrary.h"
#include "TwinStickProjectile.h"
#include "Engine/World.h"
#include "TimerManager.h"

/**
 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
 */
ATwinStickCharacter::ATwinStickCharacter()
{
 	PrimaryActorTick.bCanEverTick = true;

	// create the spring arm
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Spring Arm"));
	SpringArm->SetupAttachment(RootComponent);

	SpringArm->SetRelativeRotation(FRotator(-50.0f, 0.0f, 0.0f));

	SpringArm->TargetArmLength = 2200.0f;
	SpringArm->bDoCollisionTest = false;
	SpringArm->bInheritYaw = false;
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 0.5f;

	// create the camera
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);

	Camera->SetFieldOfView(75.0f);

	// configure the character movement
	GetCharacterMovement()->GravityScale = 1.5f;
	GetCharacterMovement()->MaxAcceleration = 1000.0f;
	GetCharacterMovement()->BrakingFrictionFactor = 1.0f;
	GetCharacterMovement()->bCanWalkOffLedges = false;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 640.0f, 0.0f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;
}

/**
 * @brief 在进入世界后解析运行时依赖、绑定事件并启动所需计时器；构造阶段不访问 World 或玩家对象。
 */
void ATwinStickCharacter::BeginPlay()
{
	Super::BeginPlay();

	// update the items count
	UpdateItems();
}

/**
 * @brief 解除委托并清理计时器或缓存，避免关卡切换和对象销毁后继续收到回调。
 * @param EndPlayReason Unreal 提供的结束原因，用于区分销毁、关卡切换和退出。
 */
void ATwinStickCharacter::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	GetWorld()->GetTimerManager().ClearTimer(AutoFireTimer);
}

/**
 * @brief 实现 Notify Controller Changed 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 */
void ATwinStickCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// set the player controller reference
	PlayerController = Cast<APlayerController>(GetController());
}

/**
 * @brief 实现 Tick 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 * @param DeltaTime 时间值 `DeltaTime`，单位为秒。
 */
void ATwinStickCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// get the current rotation
	const FRotator OldRotation = GetActorRotation();

	// are we aiming with the mouse?
	if (bUsingMouse)
	{
		if (PlayerController)
		{
			// get the cursor world location
			FHitResult OutHit;
			PlayerController->GetHitResultUnderCursorByChannel(MouseAimTraceChannel, true, OutHit);

			// find the aim rotation
			const FRotator AimRot = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), OutHit.Location);

			// save the aim angle
			AimAngle = AimRot.Yaw;



			// update the yaw, reuse the pitch and roll
			SetActorRotation(FRotator(OldRotation.Pitch, AimAngle, OldRotation.Roll));

		}

	} else {

		// use quaternion interpolation to blend between our current rotation
		// and the desired aim rotation using the shortest path
		const FRotator TargetRot = FRotator(OldRotation.Pitch, AimAngle, OldRotation.Roll);

		SetActorRotation(TargetRot);
	}
}

/**
 * @brief 绑定 Pawn 或 Character 的输入动作语义；不在 C++ 中写死具体键位。
 * @param PlayerInputComponent 参与本次操作的运行时对象 `PlayerInputComponent`；函数会检查空值和所需接口。
 */
void ATwinStickCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// set up the enhanced input action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{

		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ATwinStickCharacter::Move);
		EnhancedInputComponent->BindAction(StickAimAction, ETriggerEvent::Triggered, this, &ATwinStickCharacter::StickAim);
		EnhancedInputComponent->BindAction(MouseAimAction, ETriggerEvent::Triggered, this, &ATwinStickCharacter::MouseAim);
		EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Triggered, this, &ATwinStickCharacter::Dash);
		EnhancedInputComponent->BindAction(ShootAction, ETriggerEvent::Triggered, this, &ATwinStickCharacter::Shoot);
		EnhancedInputComponent->BindAction(AoEAction, ETriggerEvent::Triggered, this, &ATwinStickCharacter::AoEAttack);

	}

}

/**
 * @brief 执行 Move 的玩法动作；输入层只提供语义，合法性由对应领域组件决定。
 * @param Value 本次输入、状态更新或测试使用的值。
 */
void ATwinStickCharacter::Move(const FInputActionValue& Value)
{
	// save the input vector
	FVector2D InputVector = Value.Get<FVector2D>();

	// route the input
	DoMove(InputVector.X, InputVector.Y);
}

/**
 * @brief 实现 Stick Aim 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 * @param Value 本次输入、状态更新或测试使用的值。
 */
void ATwinStickCharacter::StickAim(const FInputActionValue& Value)
{
	// get the input vector
	FVector2D InputVector = Value.Get<FVector2D>();

	// route the input
	DoAim(InputVector.X, InputVector.Y);
}

/**
 * @brief 实现 Mouse Aim 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 * @param Value 本次输入、状态更新或测试使用的值。
 */
void ATwinStickCharacter::MouseAim(const FInputActionValue& Value)
{
	// raise the mouse controls flag
	bUsingMouse = true;

	// show the mouse cursor
	if (PlayerController)
	{
		PlayerController->SetShowMouseCursor(true);
	}
}

/**
 * @brief 执行 Dash 的玩法动作；输入层只提供语义，合法性由对应领域组件决定。
 * @param Value 本次输入、状态更新或测试使用的值。
 */
void ATwinStickCharacter::Dash(const FInputActionValue& Value)
{
	// route the input
	DoDash();
}

/**
 * @brief 执行 Shoot 的玩法动作；输入层只提供语义，合法性由对应领域组件决定。
 * @param Value 本次输入、状态更新或测试使用的值。
 */
void ATwinStickCharacter::Shoot(const FInputActionValue& Value)
{
	// route the input
	DoShoot();
}

/**
 * @brief 实现 Ao EAttack 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 * @param Value 本次输入、状态更新或测试使用的值。
 */
void ATwinStickCharacter::AoEAttack(const FInputActionValue& Value)
{
	// route the input
	DoAoEAttack();
}

/**
 * @brief 实现 Do Move 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 * @param AxisX 调用方提供的 `AxisX`，只在本次操作范围内使用。
 * @param AxisY 调用方提供的 `AxisY`，只在本次操作范围内使用。
 */
void ATwinStickCharacter::DoMove(float AxisX, float AxisY)
{
	// save the input
	LastMoveInput.X = AxisX;
	LastMoveInput.Y = AxisY;

	// calculate the forward component of the input
	FRotator FlatRot = GetControlRotation();
	FlatRot.Pitch = 0.0f;

	// apply the forward input
	AddMovementInput(FlatRot.RotateVector(FVector::ForwardVector), AxisX);

	// apply the right input
	AddMovementInput(FlatRot.RotateVector(FVector::RightVector), AxisY);
}

/**
 * @brief 实现 Do Aim 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 * @param AxisX 调用方提供的 `AxisX`，只在本次操作范围内使用。
 * @param AxisY 调用方提供的 `AxisY`，只在本次操作范围内使用。
 */
void ATwinStickCharacter::DoAim(float AxisX, float AxisY)
{
	// calculate the aim angle from the inputs
	AimAngle = FMath::RadiansToDegrees(FMath::Atan2(AxisY, -AxisX));

	// lower the mouse controls flag
	bUsingMouse = false;

	// hide the mouse cursor
	if (PlayerController)
	{
		PlayerController->SetShowMouseCursor(false);
	}

	// are we on autofire cooldown?
	if (!bAutoFireActive)
	{
		// set ourselves on cooldown
		bAutoFireActive = true;

		// fire a projectile
		DoShoot();

		// schedule autofire cooldown reset
		GetWorld()->GetTimerManager().SetTimer(AutoFireTimer, this, &ATwinStickCharacter::ResetAutoFire, AutoFireDelay, false);
	}
}

/**
 * @brief 实现 Do Dash 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 */
void ATwinStickCharacter::DoDash()
{
	// calculate the launch impulse vector based on the last move input
	FVector LaunchDir = FVector::ZeroVector;

	LaunchDir.X = FMath::Clamp(LastMoveInput.X, -1.0f, 1.0f);
	LaunchDir.Y = FMath::Clamp(LastMoveInput.Y, -1.0f, 1.0f);

	// launch the character in the chosen direction
	LaunchCharacter(LaunchDir * DashImpulse, true, true);
}

/**
 * @brief 实现 Do Shoot 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 */
void ATwinStickCharacter::DoShoot()
{
	// get the actor transform
	FTransform ProjectileTransform = GetActorTransform();

	// apply the projectile spawn offset
	FVector ProjectileLocation = ProjectileTransform.GetLocation() + ProjectileTransform.GetRotation().RotateVector(FVector::ForwardVector * ProjectileOffset);
	ProjectileTransform.SetLocation(ProjectileLocation);

	// ensure we don't spawn a projectile if it collides with a wall or other nearby obstacle
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;

	ATwinStickProjectile* Projectile = GetWorld()->SpawnActor<ATwinStickProjectile>(ProjectileClass, ProjectileTransform, SpawnParams);
}

/**
 * @brief 实现 Do Ao EAttack 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 */
void ATwinStickCharacter::DoAoEAttack()
{
	// do we have enough items to do an AoE attack?
	if (Items > 0)
	{
		// get the game time
		const float GameTime = GetWorld()->GetTimeSeconds();

		// are we off AoE cooldown?
		if (GameTime - LastAoETime > AoECooldownTime)
		{
			// save the new AoE time
			LastAoETime = GameTime;

			// spawn the AoE
			ATwinStickAoEAttack* AoE = GetWorld()->SpawnActor<ATwinStickAoEAttack>(AoEAttackClass, GetActorTransform());

			// decrease the number of items
			--Items;

			// update the items count
			UpdateItems();
		}
	}
}

/**
 * @brief 处理 Handle Damage 事件，将引擎回调转换为对应领域状态更新。
 * @param Damage 调用方提供的 `Damage`，只在本次操作范围内使用。
 * @param DamageDirection 本次操作使用的计数、增量或索引 `DamageDirection`；由函数校验合法范围。
 */
void ATwinStickCharacter::HandleDamage(float Damage, const FVector& DamageDirection)
{
	// calculate the knockback vector
	FVector LaunchVector = DamageDirection;
	LaunchVector.Z = 0.0f;

	// apply knockback to the character
	LaunchCharacter(LaunchVector * KnockbackStrength, true, true);

	// pass control to BP
	BP_Damaged();
}

/**
 * @brief 实现 Add Pickup 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 */
void ATwinStickCharacter::AddPickup()
{
	// increase the item count
	++Items;

	// update the items counter
	UpdateItems();
}

/**
 * @brief 根据最新领域状态刷新 Update Items，并仅在值变化时通知订阅者。
 */
void ATwinStickCharacter::UpdateItems()
{
	// update the game mode
	if (ATwinStickGameMode* GM = Cast<ATwinStickGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->ItemUsed(Items);
	}
}

/**
 * @brief 实现 Reset Auto Fire 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 */
void ATwinStickCharacter::ResetAutoFire()
{
	// reset the autofire flag
	bAutoFireActive = false;
}

