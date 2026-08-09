#include "Stealth/LRHideComponent.h"

#include "Core/LRGameplayTags.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "State/LRStateComponent.h"
#include "Stealth/LRHidePoint.h"

ULRHideComponent::ULRHideComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULRHideComponent::BeginPlay()
{
	Super::BeginPlay();
	Character = Cast<ACharacter>(GetOwner());
	State = GetOwner() ? GetOwner()->FindComponentByClass<ULRStateComponent>() : nullptr;
	ensureMsgf(Character && State, TEXT("%s requires an ACharacter owner and State component."), *GetNameSafe(this));
}

bool ULRHideComponent::IsVisibleToGuard_Implementation(AActor* guard) const
{
	return !IsHidden();
}

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
