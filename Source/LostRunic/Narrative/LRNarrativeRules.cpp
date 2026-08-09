#include "Narrative/LRNarrativeRules.h"

#include "Data/LRContentRows.h"
#include "Narrative/LRNarrativeTypes.h"

bool LRNarrativeRules::AreConditionsMet(const FGameplayTagContainer& requiredTags,
	const FGameplayTagContainer& blockedTags, const FGameplayTagContainer& contextTags)
{
	return contextTags.HasAll(requiredTags) && !contextTags.HasAny(blockedTags);
}

void LRNarrativeRules::BuildAvailableChoices(const TArray<FLRDialogueOption>& options,
	const FGameplayTagContainer& contextTags, TArray<FLRNarrativeChoice>& outChoices)
{
	outChoices.Reset();
	for (const FLRDialogueOption& option : options)
	{
		if (!AreConditionsMet(option.RequiredTags, option.BlockedTags, contextTags))
		{
			continue;
		}

		FLRNarrativeChoice& choice = outChoices.AddDefaulted_GetRef();
		choice.ChoiceId = option.OptionId;
		choice.Text = option.Text;
		choice.NextContentId = option.NextRowId;
	}
}
