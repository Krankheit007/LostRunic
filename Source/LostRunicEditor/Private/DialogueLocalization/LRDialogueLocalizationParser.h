// Copyright LostRunic. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "DialogueLocalization/LRDialogueLocalizationTypes.h"

class USUDSScript;

/** The only project adapter allowed to depend on SUDSEditor's importer implementation types. */
class FLRDialogueLocalizationParser
{
public:
	static bool ParseFile(const FString& ScriptId, const FString& Filename, USUDSScript* ImportedScript,
		TArray<FLRDialogueLocalizationEntry>& OutEntries, TArray<FString>& OutErrors);
	static bool ValidateGenerateSpeakerLinesSetting(const FString& Source, TArray<FString>& OutErrors);
	static FString ComputeSourceHash(const FString& SourceText);

private:
	static FString ExtractTranslatorContext(const TMap<FName, FString>& Metadata);
	static void ExtractFormatArgs(const FString& SourceText, TArray<FString>& OutArgs);
};
