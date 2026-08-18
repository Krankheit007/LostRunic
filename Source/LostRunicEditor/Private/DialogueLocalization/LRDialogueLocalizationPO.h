// Copyright LostRunic. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

class FJsonValue;
class FPortableObjectFormatDOM;

class FLRDialogueLocalizationPO
{
public:
	static bool BuildWorkbookRows(const FString& ManifestFilename, const FString& POFilename,
		const FString& TargetCulture, const FString& OutputRowsFilename, FString& OutError);
	static bool ApplyImportJSON(const FString& ManifestFilename, const FString& POFilename,
		const FString& ImportFilename, const bool bStrictManifest, FString& OutError);
	static bool BuildSpeakerRows(const TArray<TSharedPtr<FJsonValue>>* Speakers,
		FPortableObjectFormatDOM& DOM, uint64 PORevision, TArray<TSharedPtr<FJsonValue>>& OutRows, FString& OutError);
	static bool ApplySpeakerRows(const TArray<TSharedPtr<FJsonValue>>& ManifestSpeakers,
		const TArray<TSharedPtr<FJsonValue>>* SpeakerRows, FPortableObjectFormatDOM& DOM,
		bool bStrictManifest, int32& InOutAppliedRows, int32& InOutRejectedRows, FString& OutError);
};
