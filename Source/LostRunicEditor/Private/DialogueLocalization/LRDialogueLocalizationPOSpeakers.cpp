// Copyright LostRunic. All Rights Reserved.
#include "DialogueLocalization/LRDialogueLocalizationPO.h"

#include "DialogueLocalization/LRDialogueLocalizationLog.h"
#include "Json.h"
#include "PortableObjectFormatDOM.h"
#include "PortableObjectPipeline.h"

namespace
{
	constexpr ELocalizedTextCollapseMode CollapseMode = ELocalizedTextCollapseMode::IdenticalTextIdAndSource;
	constexpr EPortableObjectFormat POFormat = EPortableObjectFormat::Unreal;

	TSharedPtr<FJsonObject> FindManifestSpeaker(const TArray<TSharedPtr<FJsonValue>>& Values, const FString& SpeakerId)
	{
		for (const TSharedPtr<FJsonValue>& Value : Values)
		{
			const TSharedPtr<FJsonObject> Object = Value.IsValid() ? Value->AsObject() : nullptr;
			FString CandidateId;
			if (Object.IsValid() && Object->TryGetStringField(TEXT("speakerId"), CandidateId) && CandidateId == SpeakerId)
			{
				return Object;
			}
		}
		return nullptr;
	}

	bool GetRequiredString(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, FString& OutValue, FString& OutError)
	{
		if (!Object.IsValid() || !Object->TryGetStringField(Field, OutValue))
		{
			OutError = FString::Printf(TEXT("Missing JSON field '%s'."), Field);
			return false;
		}
		return true;
	}

	void ExtractFormatArgs(const FString& Text, TSet<FString>& OutArgs)
	{
		int32 SearchFrom = 0;
		while (SearchFrom < Text.Len())
		{
			const int32 Open = Text.Find(TEXT("{"), ESearchCase::CaseSensitive, ESearchDir::FromStart, SearchFrom);
			if (Open == INDEX_NONE)
			{
				return;
			}
			const int32 Close = Text.Find(TEXT("}"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Open + 1);
			if (Close == INDEX_NONE)
			{
				return;
			}
			OutArgs.Add(Text.Mid(Open + 1, Close - Open - 1));
			SearchFrom = Close + 1;
		}
	}

	bool ValidateFormatArgs(const FString& Source, const FString& Translation)
	{
		TSet<FString> SourceArgs;
		TSet<FString> TranslationArgs;
		ExtractFormatArgs(Source, SourceArgs);
		ExtractFormatArgs(Translation, TranslationArgs);
		return SourceArgs.Num() == TranslationArgs.Num() && SourceArgs.Includes(TranslationArgs);
	}
}

bool FLRDialogueLocalizationPO::BuildSpeakerRows(const TArray<TSharedPtr<FJsonValue>>* Speakers,
	FPortableObjectFormatDOM& DOM, const uint64 PORevision, TArray<TSharedPtr<FJsonValue>>& OutRows, FString& OutError)
{
	OutRows.Reset();
	if (!Speakers)
	{
		return true;
	}
	for (const TSharedPtr<FJsonValue>& Value : *Speakers)
	{
		const TSharedPtr<FJsonObject> Speaker = Value.IsValid() ? Value->AsObject() : nullptr;
		if (!Speaker.IsValid())
		{
			continue;
		}
		FString Namespace;
		FString Key;
		FString SourceText;
		Speaker->TryGetStringField(TEXT("locNamespace"), Namespace);
		Speaker->TryGetStringField(TEXT("locKey"), Key);
		Speaker->TryGetStringField(TEXT("sourceText"), SourceText);
		if (Namespace.IsEmpty() || Key.IsEmpty())
		{
			OutError = TEXT("Speaker has no UE Localization Namespace + Key; configure StringTable-backed speaker names first.");
			return false;
		}
		FPortableObjectEntry Basic;
		PortableObjectPipeline::PopulateBasicPOFileEntry(Basic, Namespace, Key, nullptr,
			SourceText, FString(), CollapseMode, POFormat);
		const TSharedPtr<FPortableObjectEntry> Existing = DOM.FindEntry(Basic.MsgId, Basic.MsgIdPlural, Basic.MsgCtxt);
		if (!Existing.IsValid())
		{
			FString SpeakerId;
			Speaker->TryGetStringField(TEXT("speakerId"), SpeakerId);
			OutError = FString::Printf(TEXT("Current PO has no Speaker entry for %s."), *SpeakerId);
			return false;
		}
		TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Speaker->Values)
		{
			Row->SetField(Pair.Key, Pair.Value);
		}
		Row->SetStringField(TEXT("msgCtxt"), Existing->MsgCtxt);
		Row->SetStringField(TEXT("msgId"), Existing->MsgId);
		Row->SetStringField(TEXT("translation"), Existing->MsgStr.Num() > 0 ? Existing->MsgStr[0] : FString());
		Row->SetStringField(TEXT("poRevision"), LexToString(PORevision));
		OutRows.Add(MakeShared<FJsonValueObject>(Row));
	}
	return true;
}

bool FLRDialogueLocalizationPO::ApplySpeakerRows(const TArray<TSharedPtr<FJsonValue>>& ManifestSpeakers,
	const TArray<TSharedPtr<FJsonValue>>* SpeakerRows, FPortableObjectFormatDOM& DOM, const bool bStrictManifest,
	int32& InOutAppliedRows, int32& InOutRejectedRows, FString& OutError)
{
	if (!SpeakerRows)
	{
		if (bStrictManifest && ManifestSpeakers.Num() > 0)
		{
			OutError = TEXT("StrictManifest rejected workbook: Speakers sheet is missing.");
			return false;
		}
		return true;
	}
	if (bStrictManifest && SpeakerRows->Num() != ManifestSpeakers.Num())
	{
		OutError = TEXT("StrictManifest rejected workbook: Speaker row count is incomplete.");
		return false;
	}
	for (const TSharedPtr<FJsonValue>& Value : *SpeakerRows)
	{
		const TSharedPtr<FJsonObject> Row = Value.IsValid() ? Value->AsObject() : nullptr;
		if (!Row.IsValid())
		{
			++InOutRejectedRows;
			continue;
		}
		FString SpeakerId;
		FString SourceHash;
		FString Namespace;
		FString Key;
		FString MsgCtxt;
		FString MsgId;
		FString Translation;
		if (!GetRequiredString(Row, TEXT("speakerId"), SpeakerId, OutError)
			|| !GetRequiredString(Row, TEXT("sourceHash"), SourceHash, OutError)
			|| !GetRequiredString(Row, TEXT("locNamespace"), Namespace, OutError)
			|| !GetRequiredString(Row, TEXT("locKey"), Key, OutError)
			|| !GetRequiredString(Row, TEXT("msgCtxt"), MsgCtxt, OutError)
			|| !GetRequiredString(Row, TEXT("msgId"), MsgId, OutError)
			|| !GetRequiredString(Row, TEXT("translation"), Translation, OutError))
		{
			return false;
		}
		const TSharedPtr<FJsonObject> ManifestSpeaker = FindManifestSpeaker(ManifestSpeakers, SpeakerId);
		if (!ManifestSpeaker.IsValid())
		{
			const FString Message = FString::Printf(TEXT("Import speaker row rejected: unknown SpeakerId %s."), *SpeakerId);
			if (bStrictManifest) { OutError = Message; return false; }
			UE_LOG(LogLostRunicDialogueLocalization, Warning, TEXT("%s"), *Message);
			++InOutRejectedRows;
			continue;
		}
		FString CurrentSourceHash;
		FString CurrentNamespace;
		FString CurrentKey;
		FString CurrentTextTableId;
		FString CurrentTextTableKey;
		FString RowTextTableId;
		FString RowTextTableKey;
		ManifestSpeaker->TryGetStringField(TEXT("sourceHash"), CurrentSourceHash);
		ManifestSpeaker->TryGetStringField(TEXT("locNamespace"), CurrentNamespace);
		ManifestSpeaker->TryGetStringField(TEXT("locKey"), CurrentKey);
		ManifestSpeaker->TryGetStringField(TEXT("textTableId"), CurrentTextTableId);
		ManifestSpeaker->TryGetStringField(TEXT("textTableKey"), CurrentTextTableKey);
		Row->TryGetStringField(TEXT("textTableId"), RowTextTableId);
		Row->TryGetStringField(TEXT("textTableKey"), RowTextTableKey);
		if (CurrentSourceHash != SourceHash || CurrentNamespace != Namespace || CurrentKey != Key
			|| CurrentTextTableId != RowTextTableId || CurrentTextTableKey != RowTextTableKey)
		{
			const FString Message = FString::Printf(TEXT("Import speaker row stale or identity-mismatched: %s."), *SpeakerId);
			if (bStrictManifest) { OutError = Message; return false; }
			UE_LOG(LogLostRunicDialogueLocalization, Warning, TEXT("%s"), *Message);
			++InOutRejectedRows;
			continue;
		}
		FString SourceText;
		ManifestSpeaker->TryGetStringField(TEXT("sourceText"), SourceText);
		FPortableObjectEntry Basic;
		PortableObjectPipeline::PopulateBasicPOFileEntry(Basic, Namespace, Key, nullptr,
			SourceText, FString(), CollapseMode, POFormat);
		if (Basic.MsgCtxt != MsgCtxt || Basic.MsgId != MsgId)
		{
			const FString Message = FString::Printf(TEXT("Speaker PO transport identity mismatch: %s."), *SpeakerId);
			if (bStrictManifest) { OutError = Message; return false; }
			UE_LOG(LogLostRunicDialogueLocalization, Warning, TEXT("%s"), *Message);
			++InOutRejectedRows;
			continue;
		}
		const TSharedPtr<FPortableObjectEntry> POEntry = DOM.FindEntry(MsgId, Basic.MsgIdPlural, MsgCtxt);
		if (!POEntry.IsValid())
		{
			const FString Message = FString::Printf(TEXT("Current PO has no Speaker entry for %s."), *SpeakerId);
			if (bStrictManifest) { OutError = Message; return false; }
			UE_LOG(LogLostRunicDialogueLocalization, Warning, TEXT("%s"), *Message);
			++InOutRejectedRows;
			continue;
		}
		if (!ValidateFormatArgs(SourceText, Translation))
		{
			const FString Message = FString::Printf(TEXT("Speaker translation format arguments mismatch: %s."), *SpeakerId);
			if (bStrictManifest) { OutError = Message; return false; }
			UE_LOG(LogLostRunicDialogueLocalization, Warning, TEXT("%s"), *Message);
			++InOutRejectedRows;
			continue;
		}
		POEntry->MsgStr.SetNum(1);
		POEntry->MsgStr[0] = Translation;
		++InOutAppliedRows;
	}
	return true;
}
