#include "Data/LRItemDefinition.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

FPrimaryAssetId ULRItemDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("LRItem"), ItemId);
}

#if WITH_EDITOR
EDataValidationResult ULRItemDefinition::IsDataValid(FDataValidationContext& context) const
{
	if (ItemId.IsNone())
	{
		context.AddError(FText::FromString(TEXT("ItemId must be a stable, non-empty name.")));
		return EDataValidationResult::Invalid;
	}

	return Super::IsDataValid(context);
}
#endif
