#include "Items/LRCourageResponseComponent.h"

#include "Core/LRGameplayTags.h"
#include "Data/LRItemDefinition.h"
#include "Data/LRGameTuningSet.h"
#include "Data/LRStateTuning.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"

ULRCourageResponseComponent::ULRCourageResponseComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

FGameplayTagContainer ULRCourageResponseComponent::GetItemUseTargetTags_Implementation()
{
	FGameplayTagContainer tags;
	tags.AddTag(bImmune ? LRGameplayTags::TargetGuardCourageImmune : LRGameplayTags::TargetGuardCourageVulnerable);
	return tags;
}

FLRItemUseResult ULRCourageResponseComponent::ApplyItemUse_Implementation(const FLRItemUseRequest& request,
	ULRItemDefinition* definition)
{
	FLRItemUseResult result;
	result.ItemId = request.ItemId;
	if (bImmune)
	{
		result.FailureReason = LRGameplayTags::ItemUseRejectImmune;
		return result;
	}

	ACharacter* character = Cast<ACharacter>(GetOwner());
	if (!character || !request.Instigator)
	{
		result.FailureReason = LRGameplayTags::ItemUseRejectExecution;
		return result;
	}
	const UGameInstance* gameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const ULRGameInstanceSubsystem* subsystem = gameInstance ? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	const ULRStateTuning* tuning = subsystem && subsystem->GetTuningSet()
		? subsystem->GetTuningSet()->State : GetDefault<ULRStateTuning>();
	const FVector direction = (character->GetActorLocation() - request.Instigator->GetActorLocation()).GetSafeNormal2D();
	TSharedPtr<FRootMotionSource_ConstantForce> knockback = MakeShared<FRootMotionSource_ConstantForce>();
	knockback->InstanceName = TEXT("LRCourageKnockback");
	knockback->Duration = tuning->CourageKnockbackDurationSeconds;
	knockback->Force = direction * tuning->CourageKnockbackSpeed;
	knockback->AccumulateMode = ERootMotionAccumulateMode::Override;
	character->GetCharacterMovement()->ApplyRootMotionSource(knockback);
	OnKnockbackApplied.Broadcast(direction);
	result.bSuccess = true;
	return result;
}
