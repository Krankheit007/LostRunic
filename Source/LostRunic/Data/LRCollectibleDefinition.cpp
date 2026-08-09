#include "Data/LRCollectibleDefinition.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

FPrimaryAssetId ULRCollectibleDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("LRCollectible"), CollectibleId);
}

#if WITH_EDITOR
EDataValidationResult ULRCollectibleDefinition::IsDataValid(FDataValidationContext& context) const
{
	if (CollectibleId.IsNone())
	{
		context.AddError(FText::FromString(TEXT("CollectibleId must be a stable, non-empty name.")));
		return EDataValidationResult::Invalid;
	}

	return Super::IsDataValid(context);
}
#endif
