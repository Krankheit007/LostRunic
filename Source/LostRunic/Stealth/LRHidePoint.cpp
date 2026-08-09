#include "Stealth/LRHidePoint.h"

#include "Components/SceneComponent.h"
#include "Core/LRGameplayTags.h"
#include "Stealth/LRHideComponent.h"

ALRHidePoint::ALRHidePoint()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	InteractionOption.ActionTag = LRGameplayTags::InteractionActionHide;
	InteractionOption.Prompt = NSLOCTEXT("LostRunic", "HidePrompt", "Hide");
}

TArray<FLRInteractionOption> ALRHidePoint::GetInteractionOptions_Implementation(AActor* interactor)
{
	return { InteractionOption };
}

FVector ALRHidePoint::GetInteractionLocation_Implementation()
{
	return GetActorLocation();
}

FLRInteractionResult ALRHidePoint::ExecuteInteraction_Implementation(AActor* interactor, const FGameplayTag actionTag)
{
	FLRInteractionResult result;
	result.ActionTag = actionTag;
	ULRHideComponent* hide = interactor ? interactor->FindComponentByClass<ULRHideComponent>() : nullptr;
	if (!hide)
	{
		result.FailureReason = LRGameplayTags::InteractionRejectState;
		return result;
	}
	result.bSuccess = hide->GetCurrentHidePoint() == this ? hide->ExitHidePoint() : hide->EnterHidePoint(this);
	if (!result.bSuccess)
	{
		result.FailureReason = LRGameplayTags::InteractionRejectState;
	}
	return result;
}
