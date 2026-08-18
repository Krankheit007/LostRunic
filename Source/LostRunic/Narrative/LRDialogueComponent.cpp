/** @file LRDialogueComponent.cpp @brief Actor-owned SUDS dialogue entry point. */
#include "Narrative/LRDialogueComponent.h"

#include "Core/LRLog.h"
#include "Engine/GameInstance.h"
#include "Narrative/LRDialogueScriptRegistry.h"
#include "Narrative/LRDialogueSubsystem.h"
#include "SUDSScript.h"

bool ULRDialogueComponent::TryStartDialogue(AActor* Instigator)
{
	if (!GetOwner() || ScriptId.IsNone())
	{
		return false;
	}
	ULRDialogueSubsystem* Dialogue = GetWorld() && GetWorld()->GetGameInstance()
		? GetWorld()->GetGameInstance()->GetSubsystem<ULRDialogueSubsystem>() : nullptr;
	const ULRDialogueScriptRegistry* Registry = Dialogue ? Dialogue->GetDialogueScriptRegistry() : nullptr;
	FString Error;
	if (!Dialogue)
	{
		UE_LOG(LogLostRunicNarrative, Warning, TEXT("Dialogue component owner=%s has no dialogue subsystem for ScriptId=%s."),
			*GetNameSafe(GetOwner()), *ScriptId.ToString());
		return false;
	}
	if (!Validate(Registry, Error))
	{
		UE_LOG(LogLostRunicNarrative, Warning, TEXT("Dialogue component owner=%s rejected ScriptId=%s: %s"),
			*GetNameSafe(GetOwner()), *ScriptId.ToString(), *Error);
		return false;
	}
	TObjectPtr<USUDSScript> Script = nullptr;
	if (!Registry->Resolve(ScriptId, Script, Error))
	{
		UE_LOG(LogLostRunicNarrative, Warning, TEXT("Dialogue component owner=%s failed to resolve ScriptId=%s: %s"),
			*GetNameSafe(GetOwner()), *ScriptId.ToString(), *Error);
		return false;
	}
	FLRDialogueStartRequest Request;
	Request.ScriptId = ScriptId;
	Request.Script = Script;
	Request.StartLabel = StartLabel;
	Request.Owner = GetOwner();
	Request.CompletionStoryTag = CompletionStoryTag;
	return Dialogue->StartSUDSDialogue(Request).bSuccess;
}

bool ULRDialogueComponent::Validate(const ULRDialogueScriptRegistry* CurrentRegistry, FString& OutError) const
{
	if (ScriptId.IsNone())
	{
		OutError = TEXT("Dialogue component requires a ScriptId.");
		return false;
	}
	if (!CurrentRegistry)
	{
		OutError = TEXT("Dialogue component cannot validate without the global DialogueScriptRegistry.");
		return false;
	}
	TObjectPtr<USUDSScript> Script = nullptr;
	if (!CurrentRegistry->Resolve(ScriptId, Script, OutError))
	{
		const FString ResolveError = OutError;
		OutError = FString::Printf(TEXT("ScriptId=%s does not resolve from the current DialogueScriptRegistry: %s"),
			*ScriptId.ToString(), *ResolveError);
		return false;
	}
	return true;
}

void ULRDialogueComponent::EndDialogue()
{
	if (ULRDialogueSubsystem* Dialogue = GetWorld() && GetWorld()->GetGameInstance()
		? GetWorld()->GetGameInstance()->GetSubsystem<ULRDialogueSubsystem>() : nullptr)
	{
		Dialogue->EndSUDSDialogue(ELRDialogueEndReason::Cancelled, GetOwner());
	}
}

void ULRDialogueComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ULRDialogueSubsystem* Dialogue = GetWorld() && GetWorld()->GetGameInstance()
		? GetWorld()->GetGameInstance()->GetSubsystem<ULRDialogueSubsystem>() : nullptr)
	{
		Dialogue->EndSUDSDialogue(ELRDialogueEndReason::OwnerDestroyed, GetOwner());
	}
	Super::EndPlay(EndPlayReason);
}
