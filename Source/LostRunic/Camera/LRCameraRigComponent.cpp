#include "Camera/LRCameraRigComponent.h"

#include "Data/LRGameTuningSet.h"
#include "Data/LRPresentationTuning.h"
#include "Engine/GameInstance.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "GameFramework/SpringArmComponent.h"

ULRCameraRigComponent::ULRCameraRigComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void ULRCameraRigComponent::BeginPlay()
{
	Super::BeginPlay();
	const UGameInstance* gameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const ULRGameInstanceSubsystem* subsystem = gameInstance ? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	const ULRPresentationTuning* tuning = subsystem && subsystem->GetTuningSet()
		? subsystem->GetTuningSet()->Presentation : GetDefault<ULRPresentationTuning>();
	if (tuning)
	{
		DefaultCameraDistance = tuning->DefaultCameraDistanceCm;
		DefaultBlendSeconds = tuning->DefaultCameraDistanceBlendSeconds;
	}
	CameraBoom = GetOwner() ? GetOwner()->FindComponentByClass<USpringArmComponent>() : nullptr;
	if (!ensureMsgf(CameraBoom, TEXT("CameraRig on %s requires a SpringArmComponent"), *GetNameSafe(GetOwner())))
	{
		return;
	}
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->TargetArmLength = DefaultCameraDistance;
	TransitionStartDistance = DefaultCameraDistance;
	TransitionTargetDistance = DefaultCameraDistance;
}

void ULRCameraRigComponent::TickComponent(const float deltaTime, const ELevelTick tickType,
	FActorComponentTickFunction* tickFunction)
{
	Super::TickComponent(deltaTime, tickType, tickFunction);
	if (!CameraBoom || TransitionDuration <= UE_KINDA_SMALL_NUMBER)
	{
		SetComponentTickEnabled(false);
		return;
	}

	TransitionElapsed += deltaTime;
	const float alpha = FMath::Clamp(TransitionElapsed / TransitionDuration, 0.0f, 1.0f);
	CameraBoom->TargetArmLength = FMath::Lerp(TransitionStartDistance, TransitionTargetDistance, alpha);
	if (alpha >= 1.0f)
	{
		TransitionDuration = 0.0f;
		SetComponentTickEnabled(false);
	}
}

bool ULRCameraRigComponent::SetSpecialCameraDistance(const float distanceCm, const float blendSeconds)
{
	if (!FMath::IsFinite(distanceCm) || distanceCm < 300.0f || distanceCm > 1400.0f || !FMath::IsFinite(blendSeconds))
	{
		return false;
	}
	StartTransition(distanceCm, blendSeconds);
	return true;
}

void ULRCameraRigComponent::RestoreDefaultCameraDistance(const float blendSeconds)
{
	if (FMath::IsFinite(blendSeconds))
	{
		StartTransition(DefaultCameraDistance, blendSeconds);
	}
}

float ULRCameraRigComponent::GetCurrentCameraDistance() const
{
	return CameraBoom ? CameraBoom->TargetArmLength : DefaultCameraDistance;
}

void ULRCameraRigComponent::StartTransition(const float targetDistance, const float blendSeconds)
{
	if (!CameraBoom)
	{
		return;
	}
	const float duration = blendSeconds < 0.0f ? DefaultBlendSeconds : blendSeconds;
	TransitionStartDistance = CameraBoom->TargetArmLength;
	TransitionTargetDistance = targetDistance;
	TransitionElapsed = 0.0f;
	TransitionDuration = duration;
	if (duration <= UE_KINDA_SMALL_NUMBER)
	{
		CameraBoom->TargetArmLength = targetDistance;
		SetComponentTickEnabled(false);
		return;
	}
	SetComponentTickEnabled(true);
}
