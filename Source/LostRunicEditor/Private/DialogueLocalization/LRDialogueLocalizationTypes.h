// Copyright LostRunic. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

/** Project-owned representation of a localizable SUDS line. SUDS parser types never leave the adapter. */
struct FLRDialogueLocalizationEntry
{
	FString ScriptId;
	FString StringKey;
	FString DisplayId;
	FString SourceText;
	FString SourceHash;
	FString TextTableId;
	FString TextTableKey;
	FString LocNamespace;
	FString LocKey;
	FString MsgCtxt;
	FString MsgId;
	FString TranslatorContext;
	FString SpeakerId;
	FString ChoicePath;
	FString ConditionalPath;
	FString EntryType;
	TArray<FString> FormatArgs;
	int32 SourceLine = 0;
	int32 Order = 0;
	bool bIsChoice = false;
};

struct FLRDialogueLocalizationSpeaker
{
	FString SpeakerId;
	FString DisplayName;
	FString SourceText;
	FString SourceHash;
	FString TextTableId;
	FString TextTableKey;
	FString LocNamespace;
	FString LocKey;
	FString MsgCtxt;
	FString MsgId;
};

struct FLRDialogueLocalizationManifest
{
	int32 SchemaVersion = 1;
	FString LocalizationTarget = TEXT("LostRunic");
	FString NativeCulture = TEXT("zh-Hans");
	FString SourceRevision;
	FString ManifestHash;
	TArray<FLRDialogueLocalizationEntry> Entries;
	TArray<FLRDialogueLocalizationSpeaker> Speakers;

	FString BuildCanonicalPayload() const;
	void RecomputeHash();
	bool Save(const FString& Filename, FString& OutError) const;
};
