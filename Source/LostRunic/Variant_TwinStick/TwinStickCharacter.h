// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @file TwinStickCharacter.h
 * @brief 保留 Unreal TwinStick 模板玩法，用于回归和 PIE 冒烟；它与 /Game/LostRunic 的“家”切片相互独立，不承载 LostRunic 核心叙事规则。
 *
 * 关联文件：TwinStickCharacter.cpp；所属领域：Variant_TwinStick。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "TwinStickCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
struct FInputActionValue;
class APlayerController;
class UInputAction;
class ATwinStickAoEAttack;
class ATwinStickProjectile;

/**
 *  A player-controlled character for a Twin Stick Shooter game
 *  Automatically rotates to face the aim direction.
 *  Fires projectiles and spawns AoE attacks.
 */
UCLASS(abstract)
class ATwinStickCharacter : public ACharacter
{
	GENERATED_BODY()

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* SpringArm;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* Camera;

protected:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* StickAimAction;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseAimAction;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* DashAction;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ShootAction;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* AoEAction;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Mouse Aim Trace Channel 的领域数据，由所属类型负责维护和校验。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, Category="Input")
	TEnumAsByte<ETraceTypeQuery> MouseAimTraceChannel;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Dash Impulse 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `2500.0f`。 可在对应资产、DataTable 行或蓝图实例中配置。编辑器约束：单位 `cm/s`，最小值 `0`，最大值 `10000`。 */
	UPROPERTY(EditAnywhere, Category="Dash", meta = (ClampMin = 0, ClampMax = 10000, Units = "cm/s"))
	float DashImpulse = 2500.0f;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Projectile Class 的软类或类默认引用，用于创建对应蓝图实例。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, Category="Projectile")
	TSubclassOf<ATwinStickProjectile> ProjectileClass;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Projectile Offset 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `100.0f`。 可在对应资产、DataTable 行或蓝图实例中配置。编辑器约束：单位 `cm`，最小值 `0`，最大值 `1000`。 */
	UPROPERTY(EditAnywhere, Category="Projectile", meta = (ClampMin = 0, ClampMax = 1000, Units = "cm"))
	float ProjectileOffset = 100.0f;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Ao EAttack Class 的软类或类默认引用，用于创建对应蓝图实例。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, Category="AoE")
	TSubclassOf<ATwinStickAoEAttack> AoEAttackClass;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Items 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `1`。 可在对应资产、DataTable 行或蓝图实例中配置。 */
	UPROPERTY(EditAnywhere, Category="AoE")
	int32 Items = 1;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Knockback Strength 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `2500.0f`。 可在对应资产、DataTable 行或蓝图实例中配置。编辑器约束：单位 `cm`，最小值 `0`，最大值 `1000`。 */
	UPROPERTY(EditAnywhere, Category="Damage", meta = (ClampMin = 0, ClampMax = 1000, Units = "cm"))
	float KnockbackStrength = 2500.0f;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Ao ECooldown Time 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `1.0f`。 可在对应资产、DataTable 行或蓝图实例中配置。编辑器约束：单位 `s`，最小值 `0`，最大值 `10`。 */
	UPROPERTY(EditAnywhere, Category="AoE", meta = (ClampMin = 0, ClampMax = 10, Units = "s"))
	float AoECooldownTime = 1.0f;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Aim Rotation Interp Speed 的移动或表现速度，默认使用厘米/秒。 C++ 安全默认值为 `10.0f`。 可在对应资产、DataTable 行或蓝图实例中配置。编辑器约束：单位 `s`，最小值 `0`，最大值 `100`。 */
	UPROPERTY(EditAnywhere, Category="Aim", meta = (ClampMin = 0, ClampMax = 100, Units = "s"))
	float AimRotationInterpSpeed = 10.0f;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Last Ao ETime 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	float LastAoETime = 0.0f;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Aim Angle 的内部运行时数据；不参与蓝图配置。 */
	float AimAngle = 0.0f;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Player Controller 的内部运行时数据；不参与蓝图配置。 */
	TObjectPtr<APlayerController> PlayerController;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Using Mouse 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	bool bUsingMouse = false;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Last Move Input 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	FVector2D LastMoveInput;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Auto Fire Active 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	bool bAutoFireActive = false;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Auto Fire Delay 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `0.2f`。 可在对应资产、DataTable 行或蓝图实例中配置。编辑器约束：单位 `s`，最小值 `0`，最大值 `5`。 */
	UPROPERTY(EditAnywhere, Category="Aim", meta = (ClampMin = 0, ClampMax = 5, Units = "s"))
	float AutoFireDelay = 0.2f;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/** Auto Fire Timer 的运行时句柄，用于取消回调并避免 Tick；不在蓝图中配置。 */
	FTimerHandle AutoFireTimer;

public:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 */
	ATwinStickCharacter();

protected:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 在进入世界后解析运行时依赖、绑定事件并启动所需计时器；构造阶段不访问 World 或玩家对象。
	 */
	virtual void BeginPlay() override;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 解除委托并清理计时器或缓存，避免关卡切换和对象销毁后继续收到回调。
	 * @param EndPlayReason Unreal 提供的结束原因，用于区分销毁、关卡切换和退出。
	 */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Notify Controller Changed 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 */
	virtual void NotifyControllerChanged() override;

public:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Tick 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 * @param DeltaTime 时间值 `DeltaTime`，单位为秒。
	 */
	virtual void Tick(float DeltaTime) override;

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 绑定 Pawn 或 Character 的输入动作语义；不在 C++ 中写死具体键位。
	 * @param PlayerInputComponent 参与本次操作的运行时对象 `PlayerInputComponent`；函数会检查空值和所需接口。
	 */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 执行 Move 的玩法动作；输入层只提供语义，合法性由对应领域组件决定。
	 * @param Value 本次输入、状态更新或测试使用的值。
	 */
	void Move(const FInputActionValue& Value);

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Stick Aim 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 * @param Value 本次输入、状态更新或测试使用的值。
	 */
	void StickAim(const FInputActionValue& Value);

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Mouse Aim 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 * @param Value 本次输入、状态更新或测试使用的值。
	 */
	void MouseAim(const FInputActionValue& Value);

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 执行 Dash 的玩法动作；输入层只提供语义，合法性由对应领域组件决定。
	 * @param Value 本次输入、状态更新或测试使用的值。
	 */
	void Dash(const FInputActionValue& Value);

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 执行 Shoot 的玩法动作；输入层只提供语义，合法性由对应领域组件决定。
	 * @param Value 本次输入、状态更新或测试使用的值。
	 */
	void Shoot(const FInputActionValue& Value);

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Ao EAttack 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 * @param Value 本次输入、状态更新或测试使用的值。
	 */
	void AoEAttack(const FInputActionValue& Value);

public:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Do Move 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 * @param AxisX 调用方提供的 `AxisX`，只在本次操作范围内使用。
	 * @param AxisY 调用方提供的 `AxisY`，只在本次操作范围内使用。
	 */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoMove(float AxisX, float AxisY);

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Do Aim 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 * @param AxisX 调用方提供的 `AxisX`，只在本次操作范围内使用。
	 * @param AxisY 调用方提供的 `AxisY`，只在本次操作范围内使用。
	 */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoAim(float AxisX, float AxisY);

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Do Dash 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoDash();

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Do Shoot 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoShoot();

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Do Ao EAttack 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoAoEAttack();

public:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 处理 Handle Damage 事件，将引擎回调转换为对应领域状态更新。
	 * @param Damage 调用方提供的 `Damage`，只在本次操作范围内使用。
	 * @param DamageDirection 本次操作使用的计数、增量或索引 `DamageDirection`；由函数校验合法范围。
	 */
	void HandleDamage(float Damage, const FVector& DamageDirection);

protected:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 BP_Damaged 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="Damage", meta = (DisplayName = "Damaged"))
	void BP_Damaged();

public:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Add Pickup 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 */
	void AddPickup();

protected:

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 根据最新领域状态刷新 Update Items，并仅在值变化时通知订阅者。
	 */
	void UpdateItems();

	/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
	/**
	 * @brief 实现 Reset Auto Fire 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 */
	void ResetAutoFire();
};
