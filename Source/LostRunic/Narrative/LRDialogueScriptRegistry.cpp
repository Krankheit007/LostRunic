/** @file LRDialogueScriptRegistry.cpp @brief Stable ScriptId to SUDS script mapping. */
#include "Narrative/LRDialogueScriptRegistry.h"

#include "SUDSScript.h"

const FLRDialogueScriptDefinition* ULRDialogueScriptRegistry::FindDefinition(const FName ScriptId) const
{
	return Scripts.FindByPredicate([ScriptId](const FLRDialogueScriptDefinition& Definition)
	{
		return Definition.ScriptId == ScriptId;
	});
}

bool ULRDialogueScriptRegistry::Resolve(const FName ScriptId, TObjectPtr<USUDSScript>& OutScript, FString& OutError) const
{
	OutScript = nullptr;
	if (ScriptId.IsNone())
	{
		OutError = TEXT("ScriptId is empty.");
		return false;
	}
	const FLRDialogueScriptDefinition* Definition = FindDefinition(ScriptId);
	if (!Definition || !Definition->Script)
	{
		OutError = FString::Printf(TEXT("ScriptId=%s is not mapped to a SUDSScript."), *ScriptId.ToString());
		return false;
	}
	OutScript = Definition->Script;
	return true;
}

bool ULRDialogueScriptRegistry::Validate(FString& OutError) const
{
	TMap<FName, TObjectPtr<USUDSScript>> ScriptsById;
	TMap<USUDSScript*, FName> IdByScript;
	for (const FLRDialogueScriptDefinition& Definition : Scripts)
	{
		if (Definition.ScriptId.IsNone() || !Definition.Script)
		{
			OutError = TEXT("Every dialogue registry entry requires a ScriptId and SUDSScript.");
			return false;
		}
		if (ScriptsById.Contains(Definition.ScriptId))
		{
			OutError = FString::Printf(TEXT("Duplicate dialogue ScriptId=%s."), *Definition.ScriptId.ToString());
			return false;
		}
		if (const FName* ExistingId = IdByScript.Find(Definition.Script))
		{
			OutError = FString::Printf(TEXT("SUDSScript=%s is registered by ScriptId=%s and ScriptId=%s."),
				*GetNameSafe(Definition.Script), *ExistingId->ToString(), *Definition.ScriptId.ToString());
			return false;
		}
		ScriptsById.Add(Definition.ScriptId, Definition.Script);
		IdByScript.Add(Definition.Script, Definition.ScriptId);
	}
	return true;
}
