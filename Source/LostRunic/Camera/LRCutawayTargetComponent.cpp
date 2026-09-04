#include "Camera/LRCutawayTargetComponent.h"

#include "LostRunic.h"
#include "Core/LRCollisionChannels.h"
#include "Core/LRCustomPrimitiveData.h"
#include "Data/LRGameTuningSet.h"
#include "Data/LRPresentationTuning.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "Components/MeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/GameInstance.h"
#include "TimerManager.h"

ULRCutawayTargetComponent::ULRCutawayTargetComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void ULRCutawayTargetComponent::BeginPlay()
{
	Super::BeginPlay();
	const UGameInstance* gameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const ULRGameInstanceSubsystem* subsystem = gameInstance ? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	Tuning = subsystem && subsystem->GetTuningSet()
		? subsystem->GetTuningSet()->Presentation.Get() : GetMutableDefault<ULRPresentationTuning>();
	if (Tuning)
	{
		HideDuration = Tuning->CutawayHideDurationSeconds;
		RestoreDuration = Tuning->CutawayRestoreDurationSeconds;
	}
	ResolvePrimitives();
	ConfigureDetectionPrimitives();
	InitializePrimitiveData();
	ValidateMaterials();
	if (bEnableForegroundCutaway)
	{
		SetCutawayRequest(this, ELRCutawayRequestType::Foreground, 1.0f, true);
	}
}

void ULRCutawayTargetComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(RequesterAuditTimer);
	LocalRequests.Empty();
	GroupRequests.Empty();
	ForegroundRequests.Empty();
	Super::EndPlay(endPlayReason);
}

void ULRCutawayTargetComponent::TickComponent(const float deltaTime, const ELevelTick tickType,
	FActorComponentTickFunction* tickFunction)
{
	Super::TickComponent(deltaTime, tickType, tickFunction);
	const double now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	const bool bLocalChanged = LocalState.Evaluate(now);
	const bool bGroupChanged = GroupState.Evaluate(now);
	const bool bForegroundChanged = ForegroundState.Evaluate(now);
	if (bLocalChanged) WriteAmount(ELRCutawayRequestType::Local, LocalState.CurrentAmount);
	if (bGroupChanged) WriteAmount(ELRCutawayRequestType::Group, GroupState.CurrentAmount);
	if (bForegroundChanged) WriteAmount(ELRCutawayRequestType::Foreground, ForegroundState.CurrentAmount);
	SetComponentTickEnabled(LocalState.IsTransitioning() || GroupState.IsTransitioning() || ForegroundState.IsTransitioning());
}

float ULRCutawayTargetComponent::GetCurrentCutawayAmount(const ELRCutawayRequestType requestType) const
{
	return GetChannel(requestType).CurrentAmount;
}

void ULRCutawayTargetComponent::SetLocalCenterUV(const FVector2D& centerUV)
{
	WritePrimitiveDataFloat(LRCustomPrimitiveData::CenterUVX, centerUV.X);
	WritePrimitiveDataFloat(LRCustomPrimitiveData::CenterUVY, centerUV.Y);
}

void ULRCutawayTargetComponent::SetLocalRadiusRefPx(const float radiusRefPx)
{
	WritePrimitiveDataFloat(LRCustomPrimitiveData::RadiusRefPx, FMath::Max(0.0f, radiusRefPx));
}

void ULRCutawayTargetComponent::ResolvePrimitives()
{
	AffectedPrimitives.Empty();
	for (int32 index = 0; index < AffectedPrimitiveReferences.Num(); ++index)
	{
		const FComponentReference& reference = AffectedPrimitiveReferences[index];
		if (UPrimitiveComponent* primitive = Cast<UPrimitiveComponent>(reference.GetComponent(GetOwner())))
		{
			AffectedPrimitives.AddUnique(primitive);
		}
		else
		{
			UE_LOG(LogLostRunicCutaway, Warning, TEXT("%s has invalid Affected Primitive reference at index %d"),
				*GetNameSafe(GetOwner()), index);
		}
	}
	if (AffectedPrimitiveReferences.IsEmpty() && GetOwner())
	{
		TInlineComponentArray<UMeshComponent*> meshes(GetOwner());
		for (UMeshComponent* mesh : meshes)
		{
			AffectedPrimitives.Add(mesh);
		}
	}
	if (AffectedPrimitives.IsEmpty())
	{
		UE_LOG(LogLostRunicCutaway, Warning, TEXT("%s has no resolved affected primitives; cutaway writes are disabled"),
			*GetNameSafe(GetOwner()));
	}
}

void ULRCutawayTargetComponent::ConfigureDetectionPrimitives()
{
	TArray<UPrimitiveComponent*> detectionPrimitives;
	for (int32 index = 0; index < DetectionPrimitiveReferences.Num(); ++index)
	{
		const FComponentReference& reference = DetectionPrimitiveReferences[index];
		if (UPrimitiveComponent* primitive = Cast<UPrimitiveComponent>(reference.GetComponent(GetOwner())))
		{
			detectionPrimitives.AddUnique(primitive);
		}
		else
		{
			UE_LOG(LogLostRunicCutaway, Warning, TEXT("%s has invalid Detection Primitive reference at index %d"),
				*GetNameSafe(GetOwner()), index);
		}
	}
	if (DetectionPrimitiveReferences.IsEmpty())
	{
		for (UPrimitiveComponent* primitive : AffectedPrimitives) detectionPrimitives.Add(primitive);
	}
	for (UPrimitiveComponent* primitive : detectionPrimitives)
	{
		const ECollisionEnabled::Type collisionEnabled = primitive->GetCollisionEnabled();
		if (collisionEnabled == ECollisionEnabled::NoCollision || collisionEnabled == ECollisionEnabled::PhysicsOnly)
		{
			UE_LOG(LogLostRunicCutaway, Warning,
				TEXT("%s detection primitive %s must enable Query collision; existing collision mode was not changed"),
				*GetNameSafe(GetOwner()), *GetNameSafe(primitive));
		}
		primitive->SetCollisionResponseToChannel(LR::CollisionChannels::CameraCutaway, ECR_Overlap);
	}
	if (detectionPrimitives.IsEmpty())
	{
		UE_LOG(LogLostRunicCutaway, Warning, TEXT("%s has no query-enabled cutaway detection primitives"),
			*GetNameSafe(GetOwner()));
	}
}

void ULRCutawayTargetComponent::InitializePrimitiveData()
{
	for (int32 index = 0; index < LRCustomPrimitiveData::Count; ++index) WritePrimitiveDataFloat(index, 0.0f);
	if (!bOverrideRootPreserve) return;
	WritePrimitiveDataFloat(LRCustomPrimitiveData::RootOverrideAmount, 1.0f);
	for (UPrimitiveComponent* primitive : AffectedPrimitives)
	{
		const float localHeightCm = primitive->GetLocalBounds().BoxExtent.Z * 2.0f;
		const float componentScaleZ = primitive->GetComponentScale().Z;
		primitive->SetCustomPrimitiveDataFloat(LRCustomPrimitiveData::RootHeight01,
			LR::Cutaway::ConvertWorldHeightToLocalFraction(RootHeightCm, localHeightCm, componentScaleZ));
		primitive->SetCustomPrimitiveDataFloat(LRCustomPrimitiveData::RootFeather01,
			LR::Cutaway::ConvertWorldHeightToLocalFraction(RootFeatherCm, localHeightCm, componentScaleZ));
	}
}

void ULRCutawayTargetComponent::WritePrimitiveDataFloat(const int32 index, const float value)
{
	for (UPrimitiveComponent* primitive : AffectedPrimitives)
	{
		if (IsValid(primitive)) primitive->SetCustomPrimitiveDataFloat(index, value);
	}
}
