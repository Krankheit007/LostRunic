#include "Camera/LRCameraCutawayComponent.h"

#include "LostRunic.h"
#include "Camera/LRCutawayTargetComponent.h"
#include "Math/UnrealMathUtility.h"
#include "Materials/MaterialParameterCollectionInstance.h"

namespace
{
	static const FName SuppressionStateParameterNames[] =
	{
		TEXT("LocalCutawayState0"), TEXT("LocalCutawayState1"),
		TEXT("LocalCutawayState2"), TEXT("LocalCutawayState3")
	};
	static const FName SuppressionActiveCountParameterName(TEXT("LocalCutawayActiveCount"));
	static const FName SuppressionTransitionParameterName(TEXT("CutawayTransitionRefPx"));
	static constexpr float MinTransitionRefPx = 0.0f;
	static constexpr float MaxTransitionRefPx = 32.0f;

	void AppendMissingParameter(FString& missingParameters, const FName parameterName)
	{
		if (!missingParameters.IsEmpty()) missingParameters += TEXT(", ");
		missingParameters += parameterName.ToString();
	}
}

bool ULRCameraCutawayComponent::InitializeSuppressionParameters()
{
	if (!CutawayViewParameterCollectionInstance) return false;
	bool bValid = true;
	FString missingParameters;
	for (const FName parameterName : SuppressionStateParameterNames)
	{
		if (!CutawayViewParameterCollectionInstance->SetVectorParameterValue(parameterName,
			FLinearColor::Transparent))
		{
			bValid = false;
			AppendMissingParameter(missingParameters, parameterName);
		}
	}
	if (!CutawayViewParameterCollectionInstance->SetScalarParameterValue(SuppressionActiveCountParameterName, 0.0f))
	{
		bValid = false;
		AppendMissingParameter(missingParameters, SuppressionActiveCountParameterName);
	}
	float transitionRefPx = 0.0f;
	if (!CutawayViewParameterCollectionInstance->GetScalarParameterValue(
		SuppressionTransitionParameterName, transitionRefPx))
	{
		bValid = false;
		AppendMissingParameter(missingParameters, SuppressionTransitionParameterName);
	}
	else if (!FMath::IsFinite(transitionRefPx)
		|| transitionRefPx < MinTransitionRefPx || transitionRefPx > MaxTransitionRefPx)
	{
		bValid = false;
		AppendMissingParameter(missingParameters, FName(TEXT("CutawayTransitionRefPx (invalid value)")));
	}
	if (!bValid)
	{
		UE_LOG(LogLostRunicCutaway, Warning,
			TEXT("%s MPC_LR_CutawayView is missing required parameter(s): %s; outline suppression is disabled"),
			*GetNameSafe(GetOwner()), *missingParameters);
		bSuppressionWarningLogged = true;
	}
	return bValid;
}

void ULRCameraCutawayComponent::ClearPublishedSuppressionStates()
{
	if (!CutawayViewParameterCollectionInstance) return;
	bool bCleared = true;
	for (const FName parameterName : SuppressionStateParameterNames)
	{
		bCleared &= CutawayViewParameterCollectionInstance->SetVectorParameterValue(parameterName,
			FLinearColor::Transparent);
	}
	bCleared &= CutawayViewParameterCollectionInstance->SetScalarParameterValue(
		SuppressionActiveCountParameterName, 0.0f);
	if (!bCleared && !bSuppressionWarningLogged)
	{
		UE_LOG(LogLostRunicCutaway, Warning,
			TEXT("%s could not clear one or more MPC_LR_CutawayView parameters during teardown"),
			*GetNameSafe(GetOwner()));
		bSuppressionWarningLogged = true;
	}
}

void ULRCameraCutawayComponent::PublishSuppressionStates()
{
	if (!CutawayViewParameterCollectionInstance) return;
	bool bWriteSucceeded = true;
	int32 activeCount = 0;
	for (int32 slotIndex = 0; slotIndex < MaxStickySlots; ++slotIndex)
	{
		ULRCutawayTargetComponent* target = StickySlotTargets.IsValidIndex(slotIndex)
			? StickySlotTargets[slotIndex].Get() : nullptr;
		if (!IsValid(target))
		{
			bWriteSucceeded &= CutawayViewParameterCollectionInstance->SetVectorParameterValue(
				SuppressionStateParameterNames[slotIndex], FLinearColor::Transparent);
			continue;
		}
		const float amount = StickySlotAmounts[slotIndex];
		bWriteSucceeded &= CutawayViewParameterCollectionInstance->SetVectorParameterValue(
			SuppressionStateParameterNames[slotIndex],
			FLinearColor(StickySlotCenters[slotIndex].X, StickySlotCenters[slotIndex].Y,
				StickySlotRadii[slotIndex], amount));
		activeCount = slotIndex + 1;
	}
	bWriteSucceeded &= CutawayViewParameterCollectionInstance->SetScalarParameterValue(
		SuppressionActiveCountParameterName, static_cast<float>(activeCount));
	if (!bWriteSucceeded)
	{
		if (!bSuppressionWarningLogged)
		{
			UE_LOG(LogLostRunicCutaway, Warning,
				TEXT("%s failed to publish one or more MPC_LR_CutawayView parameters; outline suppression is disabled"),
				*GetNameSafe(GetOwner()));
			bSuppressionWarningLogged = true;
		}
		CutawayViewParameterCollectionInstance = nullptr;
	}
}

bool ULRCameraCutawayComponent::HasStickySlots() const
{
	for (const TObjectPtr<ULRCutawayTargetComponent>& target : StickySlotTargets)
	{
		if (IsValid(target)) return true;
	}
	return false;
}

bool ULRCameraCutawayComponent::IsStickyTarget(const ULRCutawayTargetComponent* target) const
{
	if (!target) return false;
	for (const TObjectPtr<ULRCutawayTargetComponent>& slotTarget : StickySlotTargets)
	{
		if (slotTarget == target) return true;
	}
	return false;
}
