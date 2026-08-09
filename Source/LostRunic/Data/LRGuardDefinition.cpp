#include "Data/LRGuardDefinition.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

FPrimaryAssetId ULRGuardDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("LRGuard"), GuardId);
}

#if WITH_EDITOR
EDataValidationResult ULRGuardDefinition::IsDataValid(FDataValidationContext& context) const
{
	if (GuardId.IsNone() || !Tuning)
	{
		context.AddError(FText::FromString(TEXT("GuardId and Tuning are required.")));
		return EDataValidationResult::Invalid;
	}

	return Super::IsDataValid(context);
}
#endif
