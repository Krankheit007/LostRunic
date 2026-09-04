#include "Camera/LRCutawayRegion.h"

#include "Camera/LRCutawayTargetComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"

ALRCutawayRegion::ALRCutawayRegion()
{
	PrimaryActorTick.bCanEverTick = false;
	RegionBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("RegionBounds"));
	SetRootComponent(RegionBounds);
	RegionBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	RegionBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
	RegionBounds->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	RegionBounds->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::HandleBeginOverlap);
	RegionBounds->OnComponentEndOverlap.AddDynamic(this, &ThisClass::HandleEndOverlap);
}

void ALRCutawayRegion::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	SetTargetsRequested(false);
	PlayerOverlapCounts.Empty();
	Super::EndPlay(endPlayReason);
}

void ALRCutawayRegion::HandleBeginOverlap(UPrimitiveComponent* overlappedComponent, AActor* otherActor,
	UPrimitiveComponent* otherComponent, int32 otherBodyIndex, bool bFromSweep, const FHitResult& sweepResult)
{
	(void)overlappedComponent;
	(void)otherComponent;
	(void)otherBodyIndex;
	(void)bFromSweep;
	(void)sweepResult;
	if (ACharacter* character = Cast<ACharacter>(otherActor); character && character->IsPlayerControlled())
	{
		const bool bWasEmpty = PlayerOverlapCounts.IsEmpty();
		++PlayerOverlapCounts.FindOrAdd(character);
		if (bWasEmpty) SetTargetsRequested(true);
	}
}

void ALRCutawayRegion::HandleEndOverlap(UPrimitiveComponent* overlappedComponent, AActor* otherActor,
	UPrimitiveComponent* otherComponent, int32 otherBodyIndex)
{
	(void)overlappedComponent;
	(void)otherComponent;
	(void)otherBodyIndex;
	ACharacter* character = Cast<ACharacter>(otherActor);
	if (!character) return;
	if (int32* overlapCount = PlayerOverlapCounts.Find(character))
	{
		--(*overlapCount);
		if (*overlapCount <= 0) PlayerOverlapCounts.Remove(character);
		if (PlayerOverlapCounts.IsEmpty()) SetTargetsRequested(false);
	}
}

void ALRCutawayRegion::SetTargetsRequested(const bool bRequested)
{
	for (ULRCutawayTargetComponent* target : Targets)
	{
		if (!IsValid(target)) continue;
		if (bRequested) target->SetCutawayRequest(this, ELRCutawayRequestType::Group, 1.0f);
		else target->ClearCutawayRequest(this, ELRCutawayRequestType::Group);
	}
}
