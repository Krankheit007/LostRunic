#include "Data/LRGameTuningSet.h"

#include "Core/LRLog.h"
#include "Data/LRGuardTuning.h"
#include "Data/LRInteractionTuning.h"
#include "Data/LRMovementTuning.h"
#include "Data/LRPresentationTuning.h"
#include "Data/LRSaveTuning.h"
#include "Data/LRStateTuning.h"
#include "Data/LRUITuning.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

namespace
{
	template <typename T>
	bool ValidateEntry(const TCHAR* label, const T* tuning, FString& outError)
	{
		if (!tuning)
		{
			outError = FString::Printf(TEXT("Tuning set is missing required %s asset."), label);
			return false;
		}

		FString entryError;
		if (!tuning->Validate(entryError))
		{
			outError = FString::Printf(TEXT("%s tuning is invalid: %s"), label, *entryError);
			return false;
		}

		return true;
	}
}

bool ULRGameTuningSet::Validate(FString& outError) const
{
	return ValidateEntry(TEXT("State"), State.Get(), outError)
		&& ValidateEntry(TEXT("Movement"), Movement.Get(), outError)
		&& ValidateEntry(TEXT("Interaction"), Interaction.Get(), outError)
		&& ValidateEntry(TEXT("Guard"), Guard.Get(), outError)
		&& ValidateEntry(TEXT("Save"), Save.Get(), outError)
		&& ValidateEntry(TEXT("UI"), UI.Get(), outError)
		&& ValidateEntry(TEXT("Presentation"), Presentation.Get(), outError);
}

void ULRGameTuningSet::LogSources() const
{
	UE_LOG(LogLostRunicTuning, Display, TEXT("TuningSet=%s State=%s Movement=%s Interaction=%s Guard=%s Save=%s UI=%s Presentation=%s"),
		*GetPathName(), *GetNameSafe(State), *GetNameSafe(Movement), *GetNameSafe(Interaction), *GetNameSafe(Guard),
		*GetNameSafe(Save), *GetNameSafe(UI), *GetNameSafe(Presentation));
}

#if WITH_EDITOR
EDataValidationResult ULRGameTuningSet::IsDataValid(FDataValidationContext& context) const
{
	FString error;
	if (!Validate(error))
	{
		context.AddError(FText::FromString(error));
		return EDataValidationResult::Invalid;
	}

	return Super::IsDataValid(context);
}
#endif
