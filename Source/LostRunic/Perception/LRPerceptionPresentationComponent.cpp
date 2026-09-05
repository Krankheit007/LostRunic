/**
 * @file LRPerceptionPresentationComponent.cpp
 * @brief Implements Perception post-process lifetime, world style resolution and Echo slot timing.
 */
#include "Perception/LRPerceptionPresentationComponent.h"

#include "Camera/CameraComponent.h"
#include "Core/LRLog.h"
#include "Data/LRGameContentSet.h"
#include "Data/LRGameTuningSet.h"
#include "Data/LRPresentationTuning.h"
#include "Data/LRVisualStyleDefinition.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Perception/LRPerceptionEventSubsystem.h"
#include "Perception/LRPerceptionMaterialParameters.h"
#include "State/LRStateComponent.h"
#include "State/LRStatePresentationComponent.h"

ULRPerceptionPresentationComponent::ULRPerceptionPresentationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
	EchoSlots.SetNum(MaxEchoSlots);
}

void ULRPerceptionPresentationComponent::BeginPlay()
{
	Super::BeginPlay();
	ResolveRuntimeDependencies();
	if (!StateComponent || !StatePresentation || !GetWorld())
	{
		return;
	}
	StateComponent->OnHoldStarted.AddDynamic(this,
		&ULRPerceptionPresentationComponent::HandleStateHoldStarted);
	StateComponent->OnHoldCanceled.AddDynamic(this,
		&ULRPerceptionPresentationComponent::HandleStateHoldCanceled);
	StateComponent->OnStateChangeRejected.AddDynamic(this,
		&ULRPerceptionPresentationComponent::HandleStateChangeRejected);
	StatePresentation->OnStatePresentationRequested.AddDynamic(this,
		&ULRPerceptionPresentationComponent::HandleStatePresentationRequested);
	if (ACharacter* character = Cast<ACharacter>(GetOwner()))
	{
		character->OnCharacterMovementUpdated.AddDynamic(this,
			&ULRPerceptionPresentationComponent::HandleCharacterMovementUpdated);
	}
	if (ULRPerceptionEventSubsystem* subsystem = GetWorld()->GetSubsystem<ULRPerceptionEventSubsystem>())
	{
		subsystem->OnPerceptionPulse.AddUObject(this, &ULRPerceptionPresentationComponent::HandlePerceptionPulse);
	}
	SetPlayerPosition(GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector);
	if (StateComponent->GetCurrentMode() == ELRPerceptionMode::Perception)
	{
		EnterPerception();
	}
}

void ULRPerceptionPresentationComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	if (StatePresentation)
	{
		StatePresentation->OnStatePresentationRequested.RemoveDynamic(this,
			&ULRPerceptionPresentationComponent::HandleStatePresentationRequested);
	}
	if (StateComponent)
	{
		StateComponent->OnHoldStarted.RemoveDynamic(this,
			&ULRPerceptionPresentationComponent::HandleStateHoldStarted);
		StateComponent->OnHoldCanceled.RemoveDynamic(this,
			&ULRPerceptionPresentationComponent::HandleStateHoldCanceled);
		StateComponent->OnStateChangeRejected.RemoveDynamic(this,
			&ULRPerceptionPresentationComponent::HandleStateChangeRejected);
	}
	if (ACharacter* character = Cast<ACharacter>(GetOwner()))
	{
		character->OnCharacterMovementUpdated.RemoveDynamic(this,
			&ULRPerceptionPresentationComponent::HandleCharacterMovementUpdated);
	}
	if (UWorld* world = GetWorld())
	{
		if (ULRPerceptionEventSubsystem* subsystem = world->GetSubsystem<ULRPerceptionEventSubsystem>())
		{
			subsystem->OnPerceptionPulse.RemoveAll(this);
			subsystem->SetPerceptionActive(false);
		}
	}
	RemoveBlendable();
	ClearEchoes();
	SetComponentTickEnabled(false);
	Super::EndPlay(endPlayReason);
}

void ULRPerceptionPresentationComponent::TickComponent(const float deltaTime, const ELevelTick tickType,
	FActorComponentTickFunction* tickFunction)
{
	Super::TickComponent(deltaTime, tickType, tickFunction);
	if (!bBlendActive)
	{
		SetComponentTickEnabled(false);
		return;
	}
	BlendElapsed += FMath::Max(deltaTime, 0.0f);
	const float alpha = BlendDuration > UE_SMALL_NUMBER
		? FMath::Clamp(BlendElapsed / BlendDuration, 0.0f, 1.0f) : 1.0f;
	CurrentBlend = FMath::Lerp(BlendStart, BlendTarget, alpha);
	WriteStyleParameters();
	if (alpha >= 1.0f)
	{
		CompleteBlend();
	}
}

void ULRPerceptionPresentationComponent::SetPlayerPosition(const FVector& newPosition)
{
	if (newPosition.ContainsNaN())
	{
		return;
	}
	PlayerPosition = newPosition;
	if (RuntimeMPCInstance)
	{
		RuntimeMPCInstance->SetVectorParameterValue(FName(LRPerceptionMaterialParameters::PlayerPosition),
			FLinearColor(PlayerPosition.X, PlayerPosition.Y, PlayerPosition.Z, 1.0f));
	}
}

void ULRPerceptionPresentationComponent::ClearEchoes()
{
	for (FLRPerceptionEchoSlot& slot : EchoSlots)
	{
		slot = FLRPerceptionEchoSlot();
	}
	WriteRuntimeParameters();
}

int32 ULRPerceptionPresentationComponent::GetActiveEchoCount() const
{
	const float now = GetCurrentGameTime();
	int32 count = 0;
	for (const FLRPerceptionEchoSlot& slot : EchoSlots)
	{
		count += slot.bActive && slot.ExpireTime > now ? 1 : 0;
	}
	return count;
}

void ULRPerceptionPresentationComponent::SetDebugView(const ELRPerceptionDebugView newDebugView)
{
	DebugView = newDebugView;
	if (PerceptionPostProcessMID)
	{
		PerceptionPostProcessMID->SetScalarParameterValue(FName(LRPerceptionMaterialParameters::DebugView),
			static_cast<float>(DebugView));
	}
}

void ULRPerceptionPresentationComponent::ResolveRuntimeDependencies()
{
	StateComponent = GetOwner() ? GetOwner()->FindComponentByClass<ULRStateComponent>() : nullptr;
	StatePresentation = GetOwner() ? GetOwner()->FindComponentByClass<ULRStatePresentationComponent>() : nullptr;
	Camera = GetOwner() ? GetOwner()->FindComponentByClass<UCameraComponent>() : nullptr;
	const UGameInstance* gameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const ULRGameInstanceSubsystem* subsystem = gameInstance
		? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	Tuning = subsystem && subsystem->GetTuningSet()
		? subsystem->GetTuningSet()->Presentation.Get() : GetMutableDefault<ULRPresentationTuning>();
	ResolveVisualStyle(subsystem ? subsystem->GetContentSet() : nullptr);
	InitializePostProcess();
	if (!StateComponent)
	{
		UE_LOG(LogLostRunicState, Warning, TEXT("Perception presentation on %s has no StateComponent."),
			*GetNameSafe(GetOwner()));
	}
	if (!StatePresentation)
	{
		UE_LOG(LogLostRunicState, Warning, TEXT("Perception presentation on %s has no StatePresentationComponent."),
			*GetNameSafe(GetOwner()));
	}
	if (!Camera)
	{
		UE_LOG(LogLostRunicState, Warning, TEXT("Perception presentation on %s has no CameraComponent."),
			*GetNameSafe(GetOwner()));
	}
}

void ULRPerceptionPresentationComponent::ResolveVisualStyle(const ULRGameContentSet* contentSet)
{
	VisualStyle = contentSet
		? contentSet->ResolveVisualStyle(contentSet->FindMapIdForWorld(GetWorld())) : nullptr;
	if (!VisualStyle)
	{
		VisualStyle = GetMutableDefault<ULRVisualStyleDefinition>();
		UE_LOG(LogLostRunicTuning, Warning, TEXT("Perception presentation on %s uses VisualStyle CDO fallback."),
			*GetNameSafe(GetOwner()));
	}
}

void ULRPerceptionPresentationComponent::InitializePostProcess()
{
	if (!Tuning)
	{
		return;
	}
	if (UMaterialInterface* material = Tuning->PerceptionCompositeMaterial.LoadSynchronous())
	{
		PerceptionPostProcessMID = UMaterialInstanceDynamic::Create(material, this);
	}
	else
	{
		UE_LOG(LogLostRunicTuning, Warning, TEXT("Perception presentation on %s has no composite material."),
			*GetNameSafe(GetOwner()));
	}
	if (GetWorld())
	{
		if (Tuning->PerceptionVisualStyleParameterCollection)
		{
			VisualStyleMPCInstance = GetWorld()->GetParameterCollectionInstance(
				Tuning->PerceptionVisualStyleParameterCollection);
		}
		if (Tuning->PerceptionRuntimeParameterCollection)
		{
			RuntimeMPCInstance = GetWorld()->GetParameterCollectionInstance(
				Tuning->PerceptionRuntimeParameterCollection);
		}
	}
	WriteStyleParameters();
	WriteRuntimeParameters();
}

void ULRPerceptionPresentationComponent::BeginBlend(const float targetBlend, const float durationSeconds)
{
	BlendTarget = FMath::Clamp(targetBlend, 0.0f, 1.0f);
	BlendStart = CurrentBlend;
	BlendDuration = FMath::Max(durationSeconds, 0.0f);
	BlendElapsed = 0.0f;
	bBlendActive = !FMath::IsNearlyEqual(CurrentBlend, BlendTarget);
	if (!bBlendActive)
	{
		CompleteBlend();
		return;
	}
	SetComponentTickEnabled(true);
}

void ULRPerceptionPresentationComponent::CompleteBlend()
{
	CurrentBlend = BlendTarget;
	bBlendActive = false;
	SetComponentTickEnabled(false);
	if (VisualStyleMPCInstance)
	{
		VisualStyleMPCInstance->SetScalarParameterValue(FName(LRPerceptionMaterialParameters::StateBlend), CurrentBlend);
	}
	if (BlendTarget <= 0.0f)
	{
		RemoveBlendable();
		ClearEchoes();
		bPerceptionActive = false;
	}
}

void ULRPerceptionPresentationComponent::EnterPerception()
{
	if (bPerceptionActive)
	{
		return;
	}
	bPerceptionActive = true;
	SetPlayerPosition(GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector);
	AddBlendable();
	if (ULRPerceptionEventSubsystem* subsystem = GetWorld()
		? GetWorld()->GetSubsystem<ULRPerceptionEventSubsystem>() : nullptr)
	{
		subsystem->SetPerceptionActive(true);
	}
	WriteStyleParameters();
	BeginBlend(1.0f, Tuning ? Tuning->PerceptionEnterBlendSeconds : 0.30f);
}

void ULRPerceptionPresentationComponent::ExitPerception()
{
	if (!bPerceptionActive && CurrentBlend <= UE_SMALL_NUMBER)
	{
		return;
	}
	bPerceptionActive = false;
	if (ULRPerceptionEventSubsystem* subsystem = GetWorld()
		? GetWorld()->GetSubsystem<ULRPerceptionEventSubsystem>() : nullptr)
	{
		subsystem->SetPerceptionActive(false);
	}
	WriteStyleParameters();
	BeginBlend(0.0f, Tuning ? Tuning->PerceptionExitBlendSeconds : 0.20f);
}

void ULRPerceptionPresentationComponent::HandleStatePresentationRequested(
	const ELRPerceptionMode previousMode, const ELRPerceptionMode nextMode, const FGameplayTag reason)
{
	bNormalEyeHoldPreviewActive = false;
	(void)previousMode;
	(void)reason;
	if (nextMode == ELRPerceptionMode::Perception)
	{
		EnterPerception();
	}
	else
	{
		ExitPerception();
	}
}

void ULRPerceptionPresentationComponent::HandleCharacterMovementUpdated(const float deltaSeconds,
	const FVector oldLocation, const FVector oldVelocity)
{
	(void)deltaSeconds;
	(void)oldLocation;
	(void)oldVelocity;
	if (bPerceptionActive && GetOwner())
	{
		SetPlayerPosition(GetOwner()->GetActorLocation());
	}
}
