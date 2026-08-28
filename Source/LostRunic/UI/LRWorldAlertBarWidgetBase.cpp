/**
 * @file LRWorldAlertBarWidgetBase.cpp
 * @brief Binds world alert presentation to committed Guard Awareness.
 */
#include "UI/LRWorldAlertBarWidgetBase.h"

#include "AI/LRGuardAIController.h"
#include "AI/LRGuardCharacter.h"

void ULRWorldAlertBarWidgetBase::InitializeForGuard(ALRGuardCharacter* guard)
{
	Shutdown();
	GuardController = guard ? Cast<ALRGuardAIController>(guard->GetController()) : nullptr;
	if (GuardController.IsValid())
	{
		GuardController->OnGuardAwarenessChanged.AddDynamic(this,
			&ULRWorldAlertBarWidgetBase::HandleAwarenessChanged);
		HandleAwarenessChanged(GuardController->GetAwarenessSnapshot());
	}
}

void ULRWorldAlertBarWidgetBase::Shutdown()
{
	if (GuardController.IsValid())
	{
		GuardController->OnGuardAwarenessChanged.RemoveDynamic(this,
			&ULRWorldAlertBarWidgetBase::HandleAwarenessChanged);
	}
	GuardController.Reset();
	bHasSnapshot = false;
	CurrentSnapshot = FLRAlertSnapshot();
}

void ULRWorldAlertBarWidgetBase::NativeDestruct()
{
	Shutdown();
	Super::NativeDestruct();
}

void ULRWorldAlertBarWidgetBase::HandleAwarenessChanged(const FLRGuardAwarenessSnapshot& snapshot)
{
	const FLRAlertSnapshot nextSnapshot = snapshot.Alert;
	const bool bPresentationChanged = !bHasSnapshot
		|| CurrentSnapshot.Level != nextSnapshot.Level
		|| !FMath::IsNearlyEqual(CurrentSnapshot.Fraction, nextSnapshot.Fraction)
		|| CurrentSnapshot.Tier != nextSnapshot.Tier
		|| CurrentSnapshot.bFullAlert != nextSnapshot.bFullAlert;

	CurrentSnapshot = nextSnapshot;
	bHasSnapshot = true;
	if (bPresentationChanged)
	{
		HandleAlertSnapshotChanged(CurrentSnapshot);
	}
}
