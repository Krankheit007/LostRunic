/**
 * @file LRWorldAlertBarWidgetBase.cpp
 * @brief Binds world alert presentation to committed Guard Awareness.
 */
#include "UI/LRWorldAlertBarWidgetBase.h"

#include "AI/LRGuardAIController.h"
#include "AI/LRGuardCharacter.h"
#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/ProgressBar.h"

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
	ApplyDefaultPresentation(CurrentSnapshot);
	HandleAlertSnapshotChanged(CurrentSnapshot);
}

void ULRWorldAlertBarWidgetBase::ApplyDefaultPresentation(const FLRAlertSnapshot& snapshot)
{
	if (!WidgetTree)
	{
		return;
	}

	UProgressBar* whiteBar = WidgetTree->FindWidget<UProgressBar>(FName(TEXT("Alert_Bar_White")));
	UProgressBar* redBar = WidgetTree->FindWidget<UProgressBar>(FName(TEXT("Alert_Bar_Red")));
	const bool bWhiteAlert = snapshot.Tier == ELRGuardAlertTier::White;
	const bool bRedAlert = snapshot.Tier == ELRGuardAlertTier::Red
		|| snapshot.Tier == ELRGuardAlertTier::Full;

	if (whiteBar)
	{
		whiteBar->SetPercent(bWhiteAlert ? FMath::Clamp(snapshot.Fraction, 0.0f, 1.0f) : 0.0f);
		whiteBar->SetVisibility(bWhiteAlert ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (redBar)
	{
		redBar->SetPercent(bRedAlert ? FMath::Clamp(snapshot.Fraction, 0.0f, 1.0f) : 0.0f);
		redBar->SetVisibility(bRedAlert ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (!snapshot.bFullAlert)
		{
			redBar->SetRenderOpacity(1.0f);
		}
	}

	UWidgetAnimation* fullAlertAnimation = FindFullAlertAnimation();
	if (!fullAlertAnimation)
	{
		return;
	}
	if (snapshot.bFullAlert)
	{
		if (!IsAnimationPlaying(fullAlertAnimation))
		{
			// UMG treats zero loops as an infinite loop for this presentation effect.
			PlayAnimation(fullAlertAnimation, 0.0f, 0, EUMGSequencePlayMode::Forward, 1.0f, false);
		}
	}
	else
	{
		StopAnimation(fullAlertAnimation);
	}
}

UWidgetAnimation* ULRWorldAlertBarWidgetBase::FindFullAlertAnimation() const
{
	const UWidgetBlueprintGeneratedClass* widgetClass = GetWidgetTreeOwningClass();
	if (!widgetClass)
	{
		return nullptr;
	}

	for (const TObjectPtr<UWidgetAnimation>& animation : widgetClass->Animations)
	{
		if (animation && animation->GetFName() == FName(TEXT("Alert_Full_Red")))
		{
			return animation.Get();
		}
	}
	return nullptr;
}