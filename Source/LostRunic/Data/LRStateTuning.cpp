#include "Data/LRStateTuning.h"

#include "Core/LRValidation.h"

bool ULRStateTuning::Validate(FString& outError) const
{
	return LRValidation::RequireRange(TEXT("EnterHoldSeconds"), EnterHoldSeconds, 0.05f, 5.0f, outError)
		&& LRValidation::RequireRange(TEXT("ExitHoldSeconds"), ExitHoldSeconds, 0.05f, 5.0f, outError)
		&& LRValidation::RequireRange(TEXT("PresentationSafetyTimeoutSeconds"), PresentationSafetyTimeoutSeconds, 0.1f, 10.0f, outError)
		&& LRValidation::RequireRange(TEXT("CourageAttackCooldownSeconds"), CourageAttackCooldownSeconds, 0.0f, 10.0f, outError)
		&& LRValidation::RequireRange(TEXT("CourageKnockbackDurationSeconds"), CourageKnockbackDurationSeconds, 0.0f, 5.0f, outError)
		&& LRValidation::RequireRange(TEXT("CourageKnockbackSpeed"), CourageKnockbackSpeed, 0.0f, 3000.0f, outError);
}
