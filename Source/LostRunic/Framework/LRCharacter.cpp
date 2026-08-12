/**
 * @file LRCharacter.cpp
 * @brief 连接 LostRunic 的 Gameplay Framework：GameMode 管理单机世界规则，PlayerController 解释 Enhanced Input 与 UI 模式，Character 只组合能力组件，GameInstanceSubsystem 提供跨地图内容与调优配置。
 *
 * 关联文件：LRCharacter.h；所属领域：Framework。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Framework/LRCharacter.h"

#include "Gameplay/LRLocomotionComponent.h"
#include "Interaction/LRInteractionComponent.h"
#include "Items/LRAttackTargetResolver.h"
#include "Items/LRInventoryComponent.h"
#include "Items/LRItemActionComponent.h"
#include "Items/LRItemUseTypes.h"
#include "Stealth/LRHideComponent.h"
#include "Stealth/LRNoiseEmitterComponent.h"
#include "State/LRStateComponent.h"
#include "State/LRStatePresentationComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"

/**
 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
 */
ALRCharacter::ALRCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->TargetArmLength = 700.0f;
	CameraBoom->SetRelativeRotation(FRotator(-50.0f, 0.0f, 0.0f));
	CameraBoom->bDoCollisionTest = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;

	Locomotion = CreateDefaultSubobject<ULRLocomotionComponent>(TEXT("Locomotion"));
	State = CreateDefaultSubobject<ULRStateComponent>(TEXT("State"));
	StatePresentation = CreateDefaultSubobject<ULRStatePresentationComponent>(TEXT("StatePresentation"));
	Inventory = CreateDefaultSubobject<ULRInventoryComponent>(TEXT("Inventory"));
	ItemAction = CreateDefaultSubobject<ULRItemActionComponent>(TEXT("ItemAction"));
	AttackTargetResolver = CreateDefaultSubobject<ULRAttackTargetResolver>(TEXT("AttackTargetResolver"));
	Interaction = CreateDefaultSubobject<ULRInteractionComponent>(TEXT("Interaction"));
	Hide = CreateDefaultSubobject<ULRHideComponent>(TEXT("Hide"));
	NoiseEmitter = CreateDefaultSubobject<ULRNoiseEmitterComponent>(TEXT("NoiseEmitter"));
	StimuliSource = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("StimuliSource"));
	StimuliSource->bAutoRegister = true;
	StimuliSource->RegisterForSense(UAISense_Sight::StaticClass());
}

/**
 * @brief 把二维移动语义转换为角色世界方向输入；速度限制由 LRLocomotionComponent 和调优资产维护。
 * @param input 输入动作或数值 `input`；不包含写死的具体键位。
 */
void ALRCharacter::ApplyMoveInput(const FVector2D& input)
{
	const FRotator controlRotation = Controller ? Controller->GetControlRotation() : FRotator::ZeroRotator;
	const FRotator yawRotation(0.0f, controlRotation.Yaw, 0.0f);
	AddMovementInput(FRotationMatrix(yawRotation).GetUnitAxis(EAxis::X), input.Y);
	AddMovementInput(FRotationMatrix(yawRotation).GetUnitAxis(EAxis::Y), input.X);
}

/**
 * @brief 稳定玩法动作入口：使用物品；语义合法性由 ItemActionComponent 与统一事务决定。
 * @param itemId 物品的稳定 FName ID，用于定义查询和存档，不依赖显示名。
 * @param target 本次规则检查或操作的目标对象。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FLRItemUseResult ALRCharacter::RequestUseItem(const FName itemId, AActor* target)
{
	return ItemAction ? ItemAction->RequestUseItem(itemId, target) : FLRItemUseResult();
}

/**
 * @brief 稳定玩法动作入口：发起攻击；语义合法性由 ItemActionComponent 与统一事务决定。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FLRItemUseResult ALRCharacter::RequestAttack()
{
	return ItemAction ? ItemAction->RequestAttack() : FLRItemUseResult();
}
