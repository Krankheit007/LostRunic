#include "Data/LRLevelEventDefinition.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

FPrimaryAssetId ULRLevelEventDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("LRLevelEvent"), EventId);
}

#if WITH_EDITOR
EDataValidationResult ULRLevelEventDefinition::IsDataValid(FDataValidationContext& context) const
{
	if (EventId.IsNone())
	{
		context.AddError(FText::FromString(TEXT("EventId must be a stable, non-empty name.")));
		return EDataValidationResult::Invalid;
	}

	return Super::IsDataValid(context);
}
#endif
