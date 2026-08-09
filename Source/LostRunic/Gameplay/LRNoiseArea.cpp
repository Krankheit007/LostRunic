#include "Gameplay/LRNoiseArea.h"

#include "Components/BoxComponent.h"
#include "Gameplay/LRLocomotionComponent.h"

ALRNoiseArea::ALRNoiseArea()
{
	PrimaryActorTick.bCanEverTick = false;
	Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
	SetRootComponent(Bounds);
	Bounds->SetCollisionProfileName(TEXT("Trigger"));
	Bounds->OnComponentBeginOverlap.AddDynamic(this, &ALRNoiseArea::HandleBeginOverlap);
}

void ALRNoiseArea::HandleBeginOverlap(UPrimitiveComponent* component, AActor* otherActor, UPrimitiveComponent* otherComponent,
	const int32 otherBodyIndex, const bool bFromSweep, const FHitResult& sweepResult)
{
	if (ULRLocomotionComponent* locomotion = otherActor ? otherActor->FindComponentByClass<ULRLocomotionComponent>() : nullptr)
	{
		locomotion->SetNoiseEnvironment(Environment);
	}
}
