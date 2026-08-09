/**
 * @file LRHideComponent.cpp
 * @brief 实现固定/可移动躲藏点、守卫可见性接口和统一噪声发布，使守卫通过事件感知玩家而非轮询角色速度或修改基础视野。
 *
 * 关联文件：LRHideComponent.h；所属领域：Stealth。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Stealth/LRHideComponent.h"

#include "Core/LRGameplayTags.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "State/LRStateComponent.h"
#include "Stealth/LRHidePoint.h"

/**
 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
 */
ULRHideComponent::ULRHideComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

/**
 * @brief 在进入世界后解析运行时依赖、绑定事件并启动所需计时器；构造阶段不访问 World 或玩家对象。
 */
void ULRHideComponent::BeginPlay()
{
	Super::BeginPlay();
	Character = Cast<ACharacter>(GetOwner());
	State = GetOwner() ? GetOwner()->FindComponentByClass<ULRStateComponent>() : nullptr;
	ensureMsgf(Character && State, TEXT("%s requires an ACharacter owner and State component."), *GetNameSafe(this));
}

/**
 * @brief 判断 Is Visible To Guard_Implementation 对应条件；不产生玩法副作用。
 * @param guard 参与本次操作的运行时对象 `guard`；函数会检查空值和所需接口。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRHideComponent::IsVisibleToGuard_Implementation(AActor* guard) const
{
	return !IsHidden();
}

/**
 * @brief 进入指定躲藏点，应用固定或可移动掩体规则，并向守卫可见性接口报告隐藏。
 * @param hidePoint 参与本次操作的运行时对象 `hidePoint`；函数会检查空值和所需接口。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRHideComponent::EnterHidePoint(ALRHidePoint* hidePoint)
{
	if (!Character || !State || !hidePoint || IsHidden())
	{
		return false;
	}
	CurrentHidePoint = hidePoint;
	Character->SetActorLocation(hidePoint->GetHideLocation(), false, nullptr, ETeleportType::TeleportPhysics);
	bMovementLockedByHide = !hidePoint->AllowsMovement();
	if (bMovementLockedByHide)
	{
		Character->GetCharacterMovement()->DisableMovement();
	}
	State->SetBlockerActive(LRGameplayTags::StateBlockerHidden, true);
	OnHiddenStateChanged.Broadcast(true, hidePoint);
	return true;
}

/**
 * @brief 退出当前躲藏点，恢复移动限制和守卫可见性。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRHideComponent::ExitHidePoint()
{
	ALRHidePoint* hidePoint = CurrentHidePoint.Get();
	if (!Character || !State || !hidePoint)
	{
		return false;
	}
	CurrentHidePoint.Reset();
	Character->SetActorLocation(hidePoint->GetExitLocation(), false, nullptr, ETeleportType::TeleportPhysics);
	if (bMovementLockedByHide)
	{
		Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
	bMovementLockedByHide = false;
	State->SetBlockerActive(LRGameplayTags::StateBlockerHidden, false);
	OnHiddenStateChanged.Broadcast(false, nullptr);
	return true;
}
