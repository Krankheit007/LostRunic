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
}

void ULRWorldAlertBarWidgetBase::NativeDestruct()
{
	Shutdown();
	Super::NativeDestruct();
}

void ULRWorldAlertBarWidgetBase::HandleAwarenessChanged(const FLRGuardAwarenessSnapshot& snapshot)
{
	CurrentSnapshot = snapshot.Alert;
	HandleAlertSnapshotChanged(CurrentSnapshot);
}
