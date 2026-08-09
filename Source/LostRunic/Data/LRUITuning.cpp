#include "Data/LRUITuning.h"

#include "Core/LRValidation.h"

bool ULRUITuning::Validate(FString& outError) const
{
	return LRValidation::RequireRange(TEXT("TypewriterCharactersPerSecond"), TypewriterCharactersPerSecond, 1.0f, 240.0f, outError)
		&& LRValidation::RequireRange(TEXT("TypewriterUpdateSeconds"), TypewriterUpdateSeconds, 0.01f, 0.10f, outError)
		&& LRValidation::RequireRange(TEXT("FailureMessageSeconds"), FailureMessageSeconds, 0.1f, 10.0f, outError)
		&& LRValidation::RequireRange(TEXT("NavigationRepeatSeconds"), NavigationRepeatSeconds, 0.05f, 2.0f, outError);
}
