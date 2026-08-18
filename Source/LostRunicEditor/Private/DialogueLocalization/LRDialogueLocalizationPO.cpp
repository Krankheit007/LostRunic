// Copyright LostRunic. All Rights Reserved.
#include "DialogueLocalization/LRDialogueLocalizationPO.h"

#include "DialogueLocalization/LRDialogueLocalizationTypes.h"
#include "DialogueLocalization/LRDialogueLocalizationLog.h"
#include "PortableObjectPipeline.h"
#include "Json.h"
#include "LocalizationFileUtil.h"
#include "Misc/FileHelper.h"
#include "PortableObjectFormatDOM.h"

namespace
{
	constexpr ELocalizedTextCollapseMode CollapseMode = ELocalizedTextCollapseMode::IdenticalTextIdAndSource;
	constexpr EPortableObjectFormat POFormat = EPortableObjectFormat::Unreal;

	bool LoadJsonObject(const FString& Filename, TSharedPtr<FJsonObject>& OutObject, FString& OutError)
	{
		FString Text;
		if (!FFileHelper::LoadFileToString(Text, *Filename))
		{
			OutError = FString::Printf(TEXT("Unable to read JSON: %s"), *Filename);
			return false;
		}
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
		if (!FJsonSerializer::Deserialize(Reader, OutObject) || !OutObject.IsValid())
		{
			OutError = FString::Printf(TEXT("Invalid JSON: %s"), *Filename);
			return false;
		}
		return true;
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

	TSharedPtr<FJsonObject> FindManifestEntry(const TArray<TSharedPtr<FJsonValue>>& Values,
		const FString& ScriptId, const FString& StringKey)
	{
		for (const TSharedPtr<FJsonValue>& Value : Values)
		{
			const TSharedPtr<FJsonObject> Object = Value.IsValid() ? Value->AsObject() : nullptr;
			FString CandidateScript;
			FString CandidateKey;
			if (Object.IsValid() && Object->TryGetStringField(TEXT("scriptId"), CandidateScript)
				&& Object->TryGetStringField(TEXT("stringKey"), CandidateKey)
				&& CandidateScript == ScriptId && CandidateKey == StringKey)
			{
				return Object;
			}
		}
		return nullptr;
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

	bool SaveDOM(FPortableObjectFormatDOM& DOM, const FString& Filename, FString& OutError)
	{
		FString Output;
		DOM.SortEntries();
		DOM.ToString(Output);
		if (!FFileHelper::SaveStringToFile(Output, *Filename, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
		{
			OutError = FString::Printf(TEXT("Unable to write PO: %s"), *Filename);
			return false;
		}
		return true;
	}
}

bool FLRDialogueLocalizationPO::BuildWorkbookRows(const FString& ManifestFilename, const FString& POFilename,
	const FString& TargetCulture, const FString& OutputRowsFilename, FString& OutError)
{
	TSharedPtr<FJsonObject> Manifest;
	if (!LoadJsonObject(ManifestFilename, Manifest, OutError))
	{
		return false;
	}
	FString POText;
	if (!FFileHelper::LoadFileToString(POText, *POFilename))
	{
		OutError = FString::Printf(TEXT("Unable to read PO: %s"), *POFilename);
		return false;
	}
	FPortableObjectFormatDOM DOM(TargetCulture);
	if (!DOM.FromString(POText))
	{
		OutError = FString::Printf(TEXT("Invalid PO: %s"), *POFilename);
		return false;
	}
	const TArray<TSharedPtr<FJsonValue>>* Entries = nullptr;
	if (!Manifest->TryGetArrayField(TEXT("entries"), Entries))
	{
		OutError = TEXT("Manifest has no entries array.");
		return false;
	}
	const uint64 PORevision = UE::Localization::FileUtil::PortableObjectHash(POFilename);
	TArray<TSharedPtr<FJsonValue>> Rows;
	for (const TSharedPtr<FJsonValue>& Value : *Entries)
	{
		const TSharedPtr<FJsonObject> Entry = Value.IsValid() ? Value->AsObject() : nullptr;
		if (!Entry.IsValid())
		{
			continue;
		}
		FString Namespace;
		FString Key;
		FString SourceText;
		Entry->TryGetStringField(TEXT("locNamespace"), Namespace);
		Entry->TryGetStringField(TEXT("locKey"), Key);
		Entry->TryGetStringField(TEXT("sourceText"), SourceText);
		if (Namespace.IsEmpty() || Key.IsEmpty())
		{
			OutError = TEXT("Manifest entry has no UE Localization Namespace + Key; run SUDS reimport and gather first.");
			return false;
		}
		FPortableObjectEntry Basic;
		PortableObjectPipeline::PopulateBasicPOFileEntry(Basic, Namespace, Key, nullptr,
			SourceText, FString(), CollapseMode, POFormat);
		const TSharedPtr<FPortableObjectEntry> Existing = DOM.FindEntry(Basic.MsgId, Basic.MsgIdPlural, Basic.MsgCtxt);
		if (!Existing.IsValid())
		{
			OutError = FString::Printf(TEXT("Current PO has no Namespace + Key entry for %s:%s."), *Namespace, *Key);
			return false;
		}
		TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Entry->Values)
		{
			Row->SetField(Pair.Key, Pair.Value);
		}
		Row->SetStringField(TEXT("targetCulture"), TargetCulture);
		Row->SetStringField(TEXT("msgCtxt"), Existing->MsgCtxt);
		Row->SetStringField(TEXT("msgId"), Existing->MsgId);
		Row->SetStringField(TEXT("translation"), Existing->MsgStr.Num() > 0 ? Existing->MsgStr[0] : FString());
		Row->SetStringField(TEXT("poRevision"), LexToString(PORevision));
		Rows.Add(MakeShared<FJsonValueObject>(Row));
	}
	TArray<TSharedPtr<FJsonValue>> SpeakerRows;
	const TArray<TSharedPtr<FJsonValue>>* ManifestSpeakerValues = nullptr;
	Manifest->TryGetArrayField(TEXT("speakers"), ManifestSpeakerValues);
	if (!FLRDialogueLocalizationPO::BuildSpeakerRows(ManifestSpeakerValues, DOM, PORevision, SpeakerRows, OutError))
	{
		return false;
	}
	TSharedRef<FJsonObject> Output = MakeShared<FJsonObject>();
	Output->SetNumberField(TEXT("schemaVersion"), 1);
	FString LocalizationTarget;
	Manifest->TryGetStringField(TEXT("localizationTarget"), LocalizationTarget);
	Output->SetStringField(TEXT("localizationTarget"), LocalizationTarget);
	FString ManifestHash;
	Manifest->TryGetStringField(TEXT("manifestHash"), ManifestHash);
	Output->SetStringField(TEXT("manifestHash"), ManifestHash);
	Output->SetStringField(TEXT("targetCulture"), TargetCulture);
	Output->SetStringField(TEXT("poRevision"), LexToString(PORevision));
	Output->SetArrayField(TEXT("rows"), Rows);
	Output->SetArrayField(TEXT("speakers"), SpeakerRows);
	FString OutputText;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputText);
	FJsonSerializer::Serialize(Output, Writer);
	if (!FFileHelper::SaveStringToFile(OutputText + TEXT("\n"), *OutputRowsFilename, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		OutError = FString::Printf(TEXT("Unable to write workbook rows: %s"), *OutputRowsFilename);
		return false;
	}
	return true;
}

bool FLRDialogueLocalizationPO::ApplyImportJSON(const FString& ManifestFilename, const FString& POFilename,
	const FString& ImportFilename, const bool bStrictManifest, FString& OutError)
{
	TSharedPtr<FJsonObject> Manifest;
	TSharedPtr<FJsonObject> Import;
	if (!LoadJsonObject(ManifestFilename, Manifest, OutError) || !LoadJsonObject(ImportFilename, Import, OutError))
	{
		return false;
	}
	FString POText;
	if (!FFileHelper::LoadFileToString(POText, *POFilename))
	{
		OutError = FString::Printf(TEXT("Unable to read PO: %s"), *POFilename);
		return false;
	}
	FPortableObjectFormatDOM DOM;
	if (!DOM.FromString(POText))
	{
		OutError = FString::Printf(TEXT("Invalid PO: %s"), *POFilename);
		return false;
	}
	FString CurrentManifestHash;
	FString ImportManifestHash;
	Manifest->TryGetStringField(TEXT("manifestHash"), CurrentManifestHash);
	Import->TryGetStringField(TEXT("manifestHash"), ImportManifestHash);
	if (!bStrictManifest && CurrentManifestHash != ImportManifestHash)
	{
		UE_LOG(LogLostRunicDialogueLocalization, Warning, TEXT("Workbook is stale: manifestHash mismatch."));
	}
	if (bStrictManifest && CurrentManifestHash != ImportManifestHash)
	{
		OutError = TEXT("StrictManifest rejected workbook: manifestHash mismatch.");
		return false;
	}
	const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
	const TArray<TSharedPtr<FJsonValue>>* ManifestEntries = nullptr;
	const TArray<TSharedPtr<FJsonValue>>* SpeakerRows = nullptr;
	const TArray<TSharedPtr<FJsonValue>>* ManifestSpeakers = nullptr;
	if (!Import->TryGetArrayField(TEXT("rows"), Rows) || !Manifest->TryGetArrayField(TEXT("entries"), ManifestEntries)
		|| !Manifest->TryGetArrayField(TEXT("speakers"), ManifestSpeakers))
	{
		OutError = TEXT("Import JSON must contain rows and manifest entries; Manifest must contain speakers.");
		return false;
	}
	const uint64 CurrentPORevision = UE::Localization::FileUtil::PortableObjectHash(POFilename);
	FString ImportPORevision;
	Import->TryGetStringField(TEXT("poRevision"), ImportPORevision);
	if (ImportPORevision != LexToString(CurrentPORevision))
	{
		if (bStrictManifest)
		{
			OutError = TEXT("StrictManifest rejected workbook: poRevision mismatch.");
			return false;
		}
		UE_LOG(LogLostRunicDialogueLocalization, Warning, TEXT("Workbook is stale: poRevision mismatch."));
	}
	FString WorkbookFingerprint;
	Import->TryGetStringField(TEXT("workbookFingerprint"), WorkbookFingerprint);
	if (bStrictManifest && (WorkbookFingerprint.IsEmpty() || Rows->Num() != ManifestEntries->Num()))
	{
		OutError = TEXT("StrictManifest rejected workbook: fingerprint or complete row set is missing.");
		return false;
	}

	int32 AppliedRows = 0;
	int32 RejectedRows = 0;
	for (const TSharedPtr<FJsonValue>& Value : *Rows)
	{
		const TSharedPtr<FJsonObject> Row = Value.IsValid() ? Value->AsObject() : nullptr;
		if (!Row.IsValid())
		{
			++RejectedRows;
			continue;
		}
		FString ScriptId;
		FString StringKey;
		FString SourceHash;
		FString Namespace;
		FString Key;
		FString MsgCtxt;
		FString MsgId;
		FString Translation;
		if (!GetRequiredString(Row, TEXT("scriptId"), ScriptId, OutError)
			|| !GetRequiredString(Row, TEXT("stringKey"), StringKey, OutError)
			|| !GetRequiredString(Row, TEXT("sourceHash"), SourceHash, OutError)
			|| !GetRequiredString(Row, TEXT("locNamespace"), Namespace, OutError)
			|| !GetRequiredString(Row, TEXT("locKey"), Key, OutError)
			|| !GetRequiredString(Row, TEXT("msgCtxt"), MsgCtxt, OutError)
			|| !GetRequiredString(Row, TEXT("msgId"), MsgId, OutError)
			|| !GetRequiredString(Row, TEXT("translation"), Translation, OutError))
		{
			return false;
		}
		const TSharedPtr<FJsonObject> ManifestEntry = FindManifestEntry(*ManifestEntries, ScriptId, StringKey);
		if (!ManifestEntry.IsValid())
		{
			const FString Message = FString::Printf(TEXT("Import row rejected: unknown identity %s:%s."), *ScriptId, *StringKey);
			if (bStrictManifest) { OutError = Message; return false; }
			UE_LOG(LogLostRunicDialogueLocalization, Warning, TEXT("%s"), *Message);
			++RejectedRows;
			continue;
		}
		FString CurrentSourceHash;
		FString CurrentNamespace;
		FString CurrentKey;
		FString CurrentTextTableId;
		FString CurrentTextTableKey;
		FString RowTextTableId;
		FString RowTextTableKey;
		ManifestEntry->TryGetStringField(TEXT("sourceHash"), CurrentSourceHash);
		ManifestEntry->TryGetStringField(TEXT("locNamespace"), CurrentNamespace);
		ManifestEntry->TryGetStringField(TEXT("locKey"), CurrentKey);
		ManifestEntry->TryGetStringField(TEXT("textTableId"), CurrentTextTableId);
		ManifestEntry->TryGetStringField(TEXT("textTableKey"), CurrentTextTableKey);
		Row->TryGetStringField(TEXT("textTableId"), RowTextTableId);
		Row->TryGetStringField(TEXT("textTableKey"), RowTextTableKey);
		if (CurrentSourceHash != SourceHash || CurrentNamespace != Namespace || CurrentKey != Key
			|| CurrentTextTableId != RowTextTableId || CurrentTextTableKey != RowTextTableKey)
		{
			const FString Message = FString::Printf(TEXT("Import row stale or identity-mismatched: %s:%s."), *ScriptId, *StringKey);
			if (bStrictManifest) { OutError = Message; return false; }
			UE_LOG(LogLostRunicDialogueLocalization, Warning, TEXT("%s"), *Message);
			++RejectedRows;
			continue;
		}
		FPortableObjectEntry Basic;
		FString SourceText;
		ManifestEntry->TryGetStringField(TEXT("sourceText"), SourceText);
		PortableObjectPipeline::PopulateBasicPOFileEntry(Basic, Namespace, Key, nullptr,
			SourceText, FString(), CollapseMode, POFormat);
		if (Basic.MsgCtxt != MsgCtxt || Basic.MsgId != MsgId)
		{
			const FString Message = FString::Printf(TEXT("PO transport identity mismatch for %s:%s."), *ScriptId, *StringKey);
			if (bStrictManifest) { OutError = Message; return false; }
			UE_LOG(LogLostRunicDialogueLocalization, Warning, TEXT("%s"), *Message);
			++RejectedRows;
			continue;
		}
		const TSharedPtr<FPortableObjectEntry> POEntry = DOM.FindEntry(MsgId, Basic.MsgIdPlural, MsgCtxt);
		if (!POEntry.IsValid())
		{
			const FString Message = FString::Printf(TEXT("Current PO has no entry for %s:%s."), *MsgCtxt, *MsgId);
			if (bStrictManifest) { OutError = Message; return false; }
			UE_LOG(LogLostRunicDialogueLocalization, Warning, TEXT("%s"), *Message);
			++RejectedRows;
			continue;
		}
		if (!ValidateFormatArgs(SourceText, Translation))
		{
			const FString Message = FString::Printf(TEXT("Translation format arguments mismatch for %s:%s."), *ScriptId, *StringKey);
			if (bStrictManifest) { OutError = Message; return false; }
			UE_LOG(LogLostRunicDialogueLocalization, Warning, TEXT("%s"), *Message);
			++RejectedRows;
			continue;
		}
		POEntry->MsgStr.SetNum(1);
		POEntry->MsgStr[0] = Translation;
		++AppliedRows;
	}
	if (!FLRDialogueLocalizationPO::ApplySpeakerRows(*ManifestSpeakers, SpeakerRows, DOM,
		bStrictManifest, AppliedRows, RejectedRows, OutError))
	{
		return false;
	}
	if (AppliedRows == 0)
	{
		OutError = RejectedRows > 0 ? TEXT("No import rows were applied; all rows were stale or invalid.") : TEXT("Import JSON contained no rows.");
		return false;
	}
	return SaveDOM(DOM, POFilename, OutError);
}
