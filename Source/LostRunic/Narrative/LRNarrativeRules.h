#pragma once

#include "CoreMinimal.h"

struct FLRDialogueOption;
struct FLRNarrativeChoice;
struct FGameplayTagContainer;

/** Stateless condition and branching rules shared by dialogue and reading. */
namespace LRNarrativeRules
{
	LOSTRUNIC_API bool AreConditionsMet(const FGameplayTagContainer& requiredTags,
		const FGameplayTagContainer& blockedTags, const FGameplayTagContainer& contextTags);

	LOSTRUNIC_API void BuildAvailableChoices(const TArray<FLRDialogueOption>& options,
		const FGameplayTagContainer& contextTags, TArray<FLRNarrativeChoice>& outChoices);
}
