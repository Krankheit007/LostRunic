#include "Camera/LRCameraCutawayComponent.h"

#include "LostRunic.h"
#include "Camera/LRCutawayTargetComponent.h"
#include "Core/LRCollisionChannels.h"
#include "Core/LRCustomStencil.h"
#include "Data/LRGameTuningSet.h"
#include "Data/LRPresentationTuning.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "Stealth/LRHideComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "TimerManager.h"

ULRCameraCutawayComponent::ULRCameraCutawayComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
	StickySlotTargets.SetNum(MaxStickySlots);
	StickySlotCenters.SetNum(MaxStickySlots);
	StickySlotRadii.SetNumZeroed(MaxStickySlots);
	StickySlotAmounts.SetNumZeroed(MaxStickySlots);
}

void ULRCameraCutawayComponent::BeginPlay()
{
	Super::BeginPlay();
	Character = Cast<ACharacter>(GetOwner());
	Camera = GetOwner() ? GetOwner()->FindComponentByClass<UCameraComponent>() : nullptr;
	HideComponent = GetOwner() ? GetOwner()->FindComponentByClass<ULRHideComponent>() : nullptr;
	if (!ensureMsgf(Character && Camera, TEXT("CameraCutaway on %s requires Character and Camera components"), *GetNameSafe(GetOwner())))
	{
		return;
	}
	const UGameInstance* gameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const ULRGameInstanceSubsystem* subsystem = gameInstance ? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	const ULRPresentationTuning* tuning = subsystem && subsystem->GetTuningSet()
		? subsystem->GetTuningSet()->Presentation : GetDefault<ULRPresentationTuning>();
	if (tuning)
	{
		DetectionFrequencyHz = tuning->CutawayDetectionFrequencyHz;
		PrimarySphereRadiusCm = tuning->CutawayTraceSphereRadiusCm;
		RadiusRefPx = tuning->CutawayRadiusRefPx;
		if (UMaterialInterface* silhouetteMaterial = tuning->PlayerOcclusionPostProcessMaterial.LoadSynchronous())
		{
			Camera->PostProcessSettings.AddBlendable(silhouetteMaterial, 1.0f);
			Camera->PostProcessBlendWeight = 1.0f;
		}
		if (tuning->CutawayViewParameterCollection && GetWorld())
		{
			CutawayViewParameterCollectionInstance = GetWorld()->GetParameterCollectionInstance(
				tuning->CutawayViewParameterCollection);
		}
	}
	if (CutawayViewParameterCollectionInstance && !InitializeSuppressionParameters())
	{
		CutawayViewParameterCollectionInstance = nullptr;
	}
	if (!CutawayViewParameterCollectionInstance && !bSuppressionWarningLogged)
	{
		UE_LOG(LogLostRunicCutaway, Warning,
			TEXT("%s has no valid MPC_LR_CutawayView instance; outline suppression is disabled"),
			*GetNameSafe(GetOwner()));
		bSuppressionWarningLogged = true;
	}
	if (HideComponent)
	{
		bHardHidden = HideComponent->IsHidden();
		HideComponent->OnHiddenStateChanged.AddDynamic(this, &ThisClass::HandleHiddenStateChanged);
	}
	RegisterCharacterVisualPrimitives();
	const float interval = 1.0f / FMath::Max(DetectionFrequencyHz, 1.0f);
	GetWorld()->GetTimerManager().SetTimer(DetectionTimer, this, &ThisClass::DetectOccluders, interval, true);
	DetectOccluders();
}

void ULRCameraCutawayComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	ClearLocalRequests();
	ClearPublishedSuppressionStates();
	if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(DetectionTimer);
	if (HideComponent) HideComponent->OnHiddenStateChanged.RemoveDynamic(this, &ThisClass::HandleHiddenStateChanged);
	StickySlotTargets.Empty();
	StickySlotCenters.Empty();
	StickySlotRadii.Empty();
	StickySlotAmounts.Empty();
	CutawayViewParameterCollectionInstance = nullptr;
	Super::EndPlay(endPlayReason);
}

void ULRCameraCutawayComponent::TickComponent(const float deltaTime, const ELevelTick tickType,
	FActorComponentTickFunction* tickFunction)
{
	Super::TickComponent(deltaTime, tickType, tickFunction);
	RefreshStickySlotAmounts();
	UpdateStickySlots();
	UpdateProjectedCenter();
	for (int32 index = UpdatingTargets.Num() - 1; index >= 0; --index)
	{
		ULRCutawayTargetComponent* target = UpdatingTargets[index];
		if (!IsValid(target) || (!RequestedTargets.Contains(target)
			&& !IsStickyTarget(target)
			&& target->GetCurrentCutawayAmount(ELRCutawayRequestType::Local) <= UE_KINDA_SMALL_NUMBER))
		{
			UpdatingTargets.RemoveAtSwap(index);
		}
	}
	PublishSuppressionStates();
	SetComponentTickEnabled(!UpdatingTargets.IsEmpty() || HasStickySlots());
}

void ULRCameraCutawayComponent::DetectOccluders()
{
	RegisterCharacterVisualPrimitives();
	if (bHardHidden || !Character || !Camera || !GetWorld())
	{
		ClearLocalRequests();
		return;
	}

	const FVector start = Camera->GetComponentLocation();
	const FVector chest = GetChestLocation();
	const float halfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FCollisionQueryParams queryParams(SCENE_QUERY_STAT(CameraCutaway), false, Character);
	TSet<ULRCutawayTargetComponent*> newTargets;
	TArray<FHitResult> hits;
	GetWorld()->SweepMultiByChannel(hits, start, chest, FQuat::Identity, LR::CollisionChannels::CameraCutaway,
		FCollisionShape::MakeSphere(PrimarySphereRadiusCm), queryParams);
	CollectTraceTargets(hits, newTargets);

	hits.Reset();
	GetWorld()->LineTraceMultiByChannel(hits, start, Character->GetActorLocation() + FVector::UpVector * halfHeight,
		LR::CollisionChannels::CameraCutaway, queryParams);
	CollectTraceTargets(hits, newTargets);
	hits.Reset();
	GetWorld()->LineTraceMultiByChannel(hits, start, Character->GetActorLocation() - FVector::UpVector * halfHeight,
		LR::CollisionChannels::CameraCutaway, queryParams);
	CollectTraceTargets(hits, newTargets);
	UpdateLocalRequests(newTargets);
}

void ULRCameraCutawayComponent::CollectTraceTargets(const TArray<FHitResult>& hits,
	TSet<ULRCutawayTargetComponent*>& outTargets) const
{
	for (const FHitResult& hit : hits)
	{
		if (AActor* actor = hit.GetActor())
		{
			if (ULRCutawayTargetComponent* target = actor->FindComponentByClass<ULRCutawayTargetComponent>();
				target && target->SupportsRequest(ELRCutawayRequestType::Local))
			{
				outTargets.Add(target);
			}
		}
	}
}

void ULRCameraCutawayComponent::UpdateLocalRequests(const TSet<ULRCutawayTargetComponent*>& newTargets)
{
	for (auto iterator = RequestedTargets.CreateIterator(); iterator; ++iterator)
	{
		ULRCutawayTargetComponent* target = iterator->Get();
		if (!target || !newTargets.Contains(target))
		{
			if (target) target->ClearCutawayRequest(this, ELRCutawayRequestType::Local);
			iterator.RemoveCurrent();
		}
	}
	for (ULRCutawayTargetComponent* target : newTargets)
	{
		if (!RequestedTargets.Contains(target))
		{
			target->SetLocalRadiusRefPx(RadiusRefPx);
			target->SetCutawayRequest(this, ELRCutawayRequestType::Local, 1.0f);
			RequestedTargets.Add(target);
			UpdatingTargets.AddUnique(target);
		}
	}
	UpdateStickySlots();
	SetComponentTickEnabled(!UpdatingTargets.IsEmpty() || HasStickySlots());
}

void ULRCameraCutawayComponent::ClearLocalRequests()
{
	for (const TWeakObjectPtr<ULRCutawayTargetComponent>& target : RequestedTargets)
	{
		if (target.IsValid())
		{
			target->ClearCutawayRequest(this, ELRCutawayRequestType::Local);
			UpdatingTargets.AddUnique(target.Get());
		}
	}
	RequestedTargets.Empty();
	SetComponentTickEnabled(!UpdatingTargets.IsEmpty());
}

void ULRCameraCutawayComponent::UpdateProjectedCenter()
{
	if (!Character) return;
	APlayerController* controller = Cast<APlayerController>(Character->GetController());
	if (!controller) return;
	FVector2D screenPosition;
	int32 width = 0;
	int32 height = 0;
	controller->GetViewportSize(width, height);
	if (width <= 0 || height <= 0 || !controller->ProjectWorldLocationToScreen(GetChestLocation(), screenPosition, false)) return;
	const FVector2D centerUV(screenPosition.X / width, screenPosition.Y / height);
	for (ULRCutawayTargetComponent* target : UpdatingTargets)
	{
		if (IsValid(target)) target->SetLocalCenterUV(centerUV);
	}
	for (int32 slotIndex = 0; slotIndex < StickySlotTargets.Num(); ++slotIndex)
	{
		if (ULRCutawayTargetComponent* target = StickySlotTargets[slotIndex])
		{
			StickySlotCenters[slotIndex] = centerUV;
			target->SetLocalCenterUV(centerUV);
		}
	}
}

void ULRCameraCutawayComponent::RegisterCharacterVisualPrimitives()
{
	if (!Character) return;
	TInlineComponentArray<UPrimitiveComponent*> primitives(Character);
	for (UPrimitiveComponent* primitive : primitives)
	{
		const bool bIsBody = primitive == Character->GetMesh();
		if (!bIsBody && !primitive->ComponentHasTag(TEXT("CharacterVisual"))) continue;
		if (RegisteredVisualPrimitives.Contains(primitive)) continue;
		primitive->SetRenderCustomDepth(true);
		primitive->SetCustomDepthStencilValue(LRCustomStencil::PlayerOccluded);
		primitive->SetCustomDepthStencilWriteMask(ERendererStencilMask::ERSM_Default);
		RegisteredVisualPrimitives.Add(primitive);
	}
}

FVector ULRCameraCutawayComponent::GetChestLocation() const
{
	return Character->GetActorLocation() + FVector::UpVector * (Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 0.35f);
}

void ULRCameraCutawayComponent::HandleHiddenStateChanged(const bool bHidden, ALRHidePoint* hidePoint)
{
	(void)hidePoint;
	bHardHidden = bHidden;
	if (bHardHidden) ClearLocalRequests();
	else DetectOccluders();
}
