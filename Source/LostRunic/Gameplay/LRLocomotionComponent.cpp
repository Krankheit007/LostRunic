#include "Gameplay/LRLocomotionComponent.h"

#include "Core/LRGameplayTags.h"
#include "Core/LRLog.h"
#include "Data/LRGameTuningSet.h"
#include "Data/LRMovementTuning.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

ULRLocomotionComponent::ULRLocomotionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULRLocomotionComponent::BeginPlay()
{
	Super::BeginPlay();
	Character = Cast<ACharacter>(GetOwner());
	const UGameInstance* gameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const ULRGameInstanceSubsystem* subsystem = gameInstance ? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	Tuning = subsystem && subsystem->GetTuningSet() ? subsystem->GetTuningSet()->Movement : nullptr;
	if (!ensureMsgf(Character && Tuning, TEXT("%s requires an ACharacter owner and Movement tuning."), *GetNameSafe(this)))
	{
		return;
	}

	LastSampleLocation = Character->GetActorLocation();
	SetPace(Pace);
	GetWorld()->GetTimerManager().SetTimer(SampleTimer, this, &ULRLocomotionComponent::SampleTravelDistance,
		Tuning->SampleIntervalSeconds, true);
}

void ULRLocomotionComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(SampleTimer);
	}
	Super::EndPlay(endPlayReason);
}

void ULRLocomotionComponent::SetPace(const ELRMovementPace newPace)
{
	Pace = newPace;
	if (!Character || !Tuning)
	{
		return;
	}

	float speed = Tuning->WalkSpeed;
	if (Pace == ELRMovementPace::Sneak)
	{
		speed = Tuning->SneakSpeed;
	}
	else if (Pace == ELRMovementPace::Run)
	{
		speed = Tuning->RunSpeed;
	}
	Character->GetCharacterMovement()->MaxWalkSpeed = speed;
}

void ULRLocomotionComponent::ToggleSneak()
{
	ELRMovementPace& targetPace = Pace == ELRMovementPace::Run ? PaceBeforeRun : Pace;
	targetPace = targetPace == ELRMovementPace::Sneak ? ELRMovementPace::Walk : ELRMovementPace::Sneak;
	if (Pace != ELRMovementPace::Run)
	{
		SetPace(targetPace);
	}
}

void ULRLocomotionComponent::StartRun()
{
	if (Pace == ELRMovementPace::Run)
	{
		return;
	}

	PaceBeforeRun = Pace;
	SetPace(ELRMovementPace::Run);
}

void ULRLocomotionComponent::StopRun()
{
	if (Pace == ELRMovementPace::Run)
	{
		SetPace(PaceBeforeRun);
	}
}

void ULRLocomotionComponent::SampleTravelDistance()
{
	if (!Character || !Tuning)
	{
		return;
	}

	const FVector location = Character->GetActorLocation();
	DistanceSinceFootstep += FVector::Dist2D(location, LastSampleLocation);
	LastSampleLocation = location;
	const float stepDistance = GetStepDistance();
	if (DistanceSinceFootstep < stepDistance || Character->GetVelocity().SizeSquared2D() <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	DistanceSinceFootstep = FMath::Fmod(DistanceSinceFootstep, stepDistance);
	const FGameplayTag noiseTag = Pace == ELRMovementPace::Run ? LRGameplayTags::NoiseFootstepRun : LRGameplayTags::NoiseFootstepWalk;
	OnFootstep.Broadcast(location, GetNoiseRadius(), noiseTag);
}

float ULRLocomotionComponent::GetStepDistance() const
{
	return Pace == ELRMovementPace::Run ? Tuning->RunStepDistance : Tuning->WalkStepDistance;
}

float ULRLocomotionComponent::GetNoiseRadius() const
{
	if (Pace == ELRMovementPace::Sneak)
	{
		return NoiseEnvironment == ELRNoiseEnvironment::Outdoor ? Tuning->OutdoorSneakGuardNoiseRadius : 0.0f;
	}
	if (Pace == ELRMovementPace::Run)
	{
		return Tuning->IndoorRunNoiseRadius;
	}
	return NoiseEnvironment == ELRNoiseEnvironment::Outdoor ? Tuning->OutdoorAlertGuardNoiseRadius : Tuning->IndoorWalkNoiseRadius;
}
