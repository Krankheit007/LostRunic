// Copyright LostRunic. All Rights Reserved.
#include "DialogueLocalization/LRDialogueLocalizationParser.h"

#include "DialogueLocalization/LRDialogueLocalizationHash.h"
#include "Internationalization/Text.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "SUDSMessageLogger.h"
#include "SUDSScript.h"
#include "SUDSScriptEdge.h"
#include "SUDSScriptImporter.h"
#include "SUDSScriptNode.h"
#include "SUDSScriptNodeText.h"

namespace
{
	bool IsStableSudsKey(const FString& Key)
	{
		if (Key.Len() < 3 || !Key.StartsWith(TEXT("@")) || !Key.EndsWith(TEXT("@")))
		{
			return false;
		}
		for (int32 Index = 1; Index < Key.Len() - 1; ++Index)
		{
			if (!FChar::IsHexDigit(Key[Index]))
			{
				return false;
			}
		}
		return true;
	}

	FString EscapeJson(const FString& Value)
	{
		FString Escaped = Value;
		Escaped.ReplaceInline(TEXT("\\"), TEXT("\\\\"));
		Escaped.ReplaceInline(TEXT("\""), TEXT("\\\""));
		Escaped.ReplaceInline(TEXT("\r"), TEXT("\\r"));
		Escaped.ReplaceInline(TEXT("\n"), TEXT("\\n"));
		Escaped.ReplaceInline(TEXT("\t"), TEXT("\\t"));
		return Escaped;
	}

	void AppendJsonString(FString& Out, const FString& Value)
	{
		Out += FString::Printf(TEXT("\"%s\""), *EscapeJson(Value));
	}

	void AppendJsonField(FString& Out, const TCHAR* Name, const FString& Value, bool& bFirst)
	{
		if (!bFirst)
		{
			Out += TEXT(",");
		}
		bFirst = false;
		AppendJsonString(Out, Name);
		Out += TEXT(":");
		AppendJsonString(Out, Value);
	}

	void AppendJsonIntField(FString& Out, const TCHAR* Name, const int32 Value, bool& bFirst)
	{
		if (!bFirst)
		{
			Out += TEXT(",");
		}
		bFirst = false;
		AppendJsonString(Out, Name);
		Out += FString::Printf(TEXT(":%d"), Value);
	}

	void AppendJsonBoolField(FString& Out, const TCHAR* Name, const bool bValue, bool& bFirst)
	{
		if (!bFirst)
		{
			Out += TEXT(",");
		}
		bFirst = false;
		AppendJsonString(Out, Name);
		Out += bValue ? TEXT(":true") : TEXT(":false");
	}

	void AppendJsonArrayField(FString& Out, const TCHAR* Name, const TArray<FString>& Values, bool& bFirst)
	{
		if (!bFirst)
		{
			Out += TEXT(",");
		}
		bFirst = false;
		AppendJsonString(Out, Name);
		Out += TEXT(":");
		Out += TEXT("[");
		for (int32 Index = 0; Index < Values.Num(); ++Index)
		{
			if (Index > 0)
			{
				Out += TEXT(",");
			}
			AppendJsonString(Out, Values[Index]);
		}
		Out += TEXT("]");
	}

	FString EntrySortKey(const FLRDialogueLocalizationEntry& Entry)
	{
		return Entry.ScriptId + TEXT("\x1f") + Entry.StringKey;
	}
}

FString FLRDialogueLocalizationManifest::BuildCanonicalPayload() const
{
	TArray<FLRDialogueLocalizationEntry> SortedEntries = Entries;
	SortedEntries.Sort([](const FLRDialogueLocalizationEntry& Left, const FLRDialogueLocalizationEntry& Right)
	{
		return EntrySortKey(Left) < EntrySortKey(Right);
	});
	TArray<FLRDialogueLocalizationSpeaker> SortedSpeakers = Speakers;
	SortedSpeakers.Sort([](const FLRDialogueLocalizationSpeaker& Left, const FLRDialogueLocalizationSpeaker& Right)
	{
		return Left.SpeakerId < Right.SpeakerId;
	});

	FString Out = TEXT("{");
	bool bFirst = true;
	AppendJsonIntField(Out, TEXT("schemaVersion"), SchemaVersion, bFirst);
	AppendJsonField(Out, TEXT("localizationTarget"), LocalizationTarget, bFirst);
	AppendJsonField(Out, TEXT("nativeCulture"), NativeCulture, bFirst);
	TArray<FString> ScriptIds;
	for (const FLRDialogueLocalizationEntry& Entry : SortedEntries)
	{
		ScriptIds.AddUnique(Entry.ScriptId);
	}
	ScriptIds.Sort();
	Out += TEXT(",\"scripts\":[");
	for (int32 Index = 0; Index < ScriptIds.Num(); ++Index)
	{
		if (Index > 0)
		{
			Out += TEXT(",");
		}
		Out += TEXT("{");
		bool bScriptFirst = true;
		AppendJsonField(Out, TEXT("scriptId"), ScriptIds[Index], bScriptFirst);
		Out += TEXT("}");
	}
	Out += TEXT("],\"entries\":[");
	for (int32 Index = 0; Index < SortedEntries.Num(); ++Index)
	{
		if (Index > 0)
		{
			Out += TEXT(",");
		}
		const FLRDialogueLocalizationEntry& Entry = SortedEntries[Index];
		Out += TEXT("{");
		bool bEntryFirst = true;
		AppendJsonField(Out, TEXT("scriptId"), Entry.ScriptId, bEntryFirst);
		AppendJsonField(Out, TEXT("stringKey"), Entry.StringKey, bEntryFirst);
		AppendJsonField(Out, TEXT("displayId"), Entry.DisplayId, bEntryFirst);
		AppendJsonField(Out, TEXT("sourceText"), Entry.SourceText, bEntryFirst);
		AppendJsonField(Out, TEXT("sourceHash"), Entry.SourceHash, bEntryFirst);
		AppendJsonField(Out, TEXT("textTableId"), Entry.TextTableId, bEntryFirst);
		AppendJsonField(Out, TEXT("textTableKey"), Entry.TextTableKey, bEntryFirst);
		AppendJsonField(Out, TEXT("locNamespace"), Entry.LocNamespace, bEntryFirst);
		AppendJsonField(Out, TEXT("locKey"), Entry.LocKey, bEntryFirst);
		AppendJsonField(Out, TEXT("translatorContext"), Entry.TranslatorContext, bEntryFirst);
		AppendJsonField(Out, TEXT("speakerId"), Entry.SpeakerId, bEntryFirst);
		AppendJsonField(Out, TEXT("choicePath"), Entry.ChoicePath, bEntryFirst);
		AppendJsonField(Out, TEXT("conditionalPath"), Entry.ConditionalPath, bEntryFirst);
		AppendJsonField(Out, TEXT("entryType"), Entry.EntryType, bEntryFirst);
		AppendJsonArrayField(Out, TEXT("formatArgs"), Entry.FormatArgs, bEntryFirst);
		AppendJsonIntField(Out, TEXT("sourceLine"), Entry.SourceLine, bEntryFirst);
		AppendJsonIntField(Out, TEXT("order"), Entry.Order, bEntryFirst);
		AppendJsonBoolField(Out, TEXT("isChoice"), Entry.bIsChoice, bEntryFirst);
		Out += TEXT("}");
	}
	Out += TEXT("],\"speakers\":[");
	for (int32 Index = 0; Index < SortedSpeakers.Num(); ++Index)
	{
		if (Index > 0)
		{
			Out += TEXT(",");
		}
		const FLRDialogueLocalizationSpeaker& Speaker = SortedSpeakers[Index];
		Out += TEXT("{");
		bool bSpeakerFirst = true;
		AppendJsonField(Out, TEXT("speakerId"), Speaker.SpeakerId, bSpeakerFirst);
		AppendJsonField(Out, TEXT("displayName"), Speaker.DisplayName, bSpeakerFirst);
		AppendJsonField(Out, TEXT("sourceText"), Speaker.SourceText, bSpeakerFirst);
		AppendJsonField(Out, TEXT("sourceHash"), Speaker.SourceHash, bSpeakerFirst);
		AppendJsonField(Out, TEXT("textTableId"), Speaker.TextTableId, bSpeakerFirst);
		AppendJsonField(Out, TEXT("textTableKey"), Speaker.TextTableKey, bSpeakerFirst);
		AppendJsonField(Out, TEXT("locNamespace"), Speaker.LocNamespace, bSpeakerFirst);
		AppendJsonField(Out, TEXT("locKey"), Speaker.LocKey, bSpeakerFirst);
		AppendJsonField(Out, TEXT("msgCtxt"), Speaker.MsgCtxt, bSpeakerFirst);
		AppendJsonField(Out, TEXT("msgId"), Speaker.MsgId, bSpeakerFirst);
		Out += TEXT("}");
	}
	Out += TEXT("]}");
	return Out;
}

void FLRDialogueLocalizationManifest::RecomputeHash()
{
	ManifestHash = FLRDialogueLocalizationHash::Sha256String(BuildCanonicalPayload());
}

bool FLRDialogueLocalizationManifest::Save(const FString& Filename, FString& OutError) const
{
	FLRDialogueLocalizationManifest Copy = *this;
	Copy.RecomputeHash();
	FString Json = Copy.BuildCanonicalPayload();
	const int32 ScriptsPosition = Json.Find(TEXT(",\"scripts\":"));
	if (ScriptsPosition == INDEX_NONE)
	{
		OutError = TEXT("Canonical manifest payload has no scripts field.");
		return false;
	}
	Json = Json.Left(ScriptsPosition)
		+ FString::Printf(TEXT(",\"sourceRevision\":\"%s\",\"manifestHash\":\"%s\""),
			*EscapeJson(Copy.SourceRevision), *EscapeJson(Copy.ManifestHash))
		+ Json.Mid(ScriptsPosition);
	if (!FFileHelper::SaveStringToFile(Json + TEXT("\n"), *Filename, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		OutError = FString::Printf(TEXT("Failed to save manifest: %s"), *Filename);
		return false;
	}
	return true;
}

bool FLRDialogueLocalizationParser::ValidateGenerateSpeakerLinesSetting(const FString& Source, TArray<FString>& OutErrors)
{
	TArray<FString> Lines;
	Source.ParseIntoArrayLines(Lines, false);
	for (const FString& Line : Lines)
	{
		if (Line.Contains(TEXT("GenerateSpeakerLinesFromChoices"), ESearchCase::IgnoreCase)
			&& Line.Contains(TEXT("true"), ESearchCase::IgnoreCase))
		{
			OutErrors.Add(TEXT("GenerateSpeakerLinesFromChoices must be false for LostRunic dialogue."));
		}
	}
	return OutErrors.IsEmpty();
}

bool FLRDialogueLocalizationParser::ParseFile(const FString& ScriptId, const FString& Filename, USUDSScript* ImportedScript,
	TArray<FLRDialogueLocalizationEntry>& OutEntries, TArray<FString>& OutErrors)
{
	FString Source;
	if (!FFileHelper::LoadFileToString(Source, *Filename))
	{
		OutErrors.Add(FString::Printf(TEXT("Unable to read SUDS source: %s"), *Filename));
		return false;
	}
	ValidateGenerateSpeakerLinesSetting(Source, OutErrors);
	FSUDSMessageLogger Logger(false);
	FSUDSScriptImporter Importer;
	if (!Importer.ImportFromBuffer(*Source, Source.Len(), Filename, &Logger, true))
	{
		OutErrors.Add(FString::Printf(TEXT("SUDS parser rejected %s."), *Filename));
		return false;
	}

	TSet<FString> SeenKeys;
	int32 Order = 0;
	for (int32 NodeIndex = 0;; ++NodeIndex)
	{
		const FSUDSParsedNode* Node = Importer.GetNode(NodeIndex);
		if (!Node)
		{
			break;
		}
		if (Node->NodeType == ESUDSParsedNodeType::Text && !Node->Text.IsEmpty())
		{
			FLRDialogueLocalizationEntry& Entry = OutEntries.AddDefaulted_GetRef();
			Entry.ScriptId = ScriptId;
			Entry.StringKey = Node->TextID;
			Entry.DisplayId = ScriptId + TEXT(":") + Entry.StringKey;
			Entry.SourceText = Node->Text;
			Entry.SourceHash = ComputeSourceHash(Entry.SourceText);
			Entry.SourceLine = Node->SourceLineNo;
			Entry.Order = Order++;
			Entry.SpeakerId = Node->Identifier;
			Entry.ChoicePath = Node->ChoicePath;
			Entry.ConditionalPath = Node->ConditionalPath;
			Entry.EntryType = TEXT("SpeakerLine");
			Entry.TranslatorContext = ExtractTranslatorContext(Node->TextMetadata);
			ExtractFormatArgs(Entry.SourceText, Entry.FormatArgs);
			if (!IsStableSudsKey(Entry.StringKey) || SeenKeys.Contains(Entry.StringKey))
			{
				OutErrors.Add(FString::Printf(TEXT("Invalid or duplicate SUDS StringKey %s in %s:%d."), *Entry.StringKey, *Filename, Entry.SourceLine));
			}
			SeenKeys.Add(Entry.StringKey);
		}

		for (const FSUDSParsedEdge& Edge : Node->Edges)
		{
			if (Edge.Text.IsEmpty())
			{
				continue;
			}
			FLRDialogueLocalizationEntry& Entry = OutEntries.AddDefaulted_GetRef();
			Entry.ScriptId = ScriptId;
			Entry.StringKey = Edge.TextID;
			Entry.DisplayId = ScriptId + TEXT(":") + Entry.StringKey;
			Entry.SourceText = Edge.Text;
			Entry.SourceHash = ComputeSourceHash(Entry.SourceText);
			Entry.SourceLine = Edge.SourceLineNo;
			Entry.Order = Order++;
			Entry.SpeakerId = TEXT("Player");
			Entry.ChoicePath = Node->ChoicePath;
			Entry.ConditionalPath = Node->ConditionalPath;
			Entry.EntryType = TEXT("Choice");
			Entry.bIsChoice = true;
			Entry.TranslatorContext = ExtractTranslatorContext(Edge.TextMetadata);
			ExtractFormatArgs(Entry.SourceText, Entry.FormatArgs);
			if (!IsStableSudsKey(Entry.StringKey) || SeenKeys.Contains(Entry.StringKey))
			{
				OutErrors.Add(FString::Printf(TEXT("Invalid or duplicate SUDS StringKey %s in %s:%d."), *Entry.StringKey, *Filename, Entry.SourceLine));
			}
			SeenKeys.Add(Entry.StringKey);
		}
	}

	for (FLRDialogueLocalizationEntry& Entry : OutEntries)
	{
		if (!ImportedScript)
		{
			continue;
		}
		FText Text;
		if (USUDSScriptNodeText* TextNode = ImportedScript->GetNodeByTextID(Entry.StringKey))
		{
			Text = TextNode->GetText();
		}
		else
		{
			for (USUDSScriptNode* Node : ImportedScript->GetNodes())
			{
				for (const FSUDSScriptEdge& Edge : Node->GetEdges())
				{
					if (Edge.GetTextID() == Entry.StringKey)
					{
						Text = Edge.GetText();
						break;
					}
				}
				if (!Text.IsEmpty())
				{
					break;
				}
			}
		}
		if (!Text.IsEmpty())
		{
			FName TableId;
			FString TableKey;
			if (FTextInspector::GetTableIdAndKey(Text, TableId, TableKey))
			{
				Entry.TextTableId = TableId.ToString();
				Entry.TextTableKey = TableKey;
			}
			if (const TOptional<FString> Namespace = FTextInspector::GetNamespace(Text))
			{
				Entry.LocNamespace = Namespace.GetValue();
			}
			if (const TOptional<FString> Key = FTextInspector::GetKey(Text))
			{
				Entry.LocKey = Key.GetValue();
			}
		}
	}
	return OutErrors.IsEmpty();
}

FString FLRDialogueLocalizationParser::ComputeSourceHash(const FString& SourceText)
{
	FString Normalized = SourceText;
	Normalized.ReplaceInline(TEXT("\r\n"), TEXT("\n"));
	Normalized.ReplaceInline(TEXT("\r"), TEXT("\n"));
	FTCHARToUTF8 Utf8(*Normalized);
	return FLRDialogueLocalizationHash::Sha256(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length());
}

FString FLRDialogueLocalizationParser::ExtractTranslatorContext(const TMap<FName, FString>& Metadata)
{
	TArray<FString> Values;
	for (const TPair<FName, FString>& Pair : Metadata)
	{
		if (Pair.Key != TEXT("Speaker"))
		{
			Values.Add(Pair.Key.ToString() + TEXT(": ") + Pair.Value);
		}
	}
	Values.Sort();
	return FString::Join(Values, TEXT("\n"));
}

void FLRDialogueLocalizationParser::ExtractFormatArgs(const FString& SourceText, TArray<FString>& OutArgs)
{
	int32 SearchFrom = 0;
	while (SearchFrom < SourceText.Len())
	{
		const int32 Open = SourceText.Find(TEXT("{"), ESearchCase::CaseSensitive, ESearchDir::FromStart, SearchFrom);
		if (Open == INDEX_NONE)
		{
			break;
		}
		const int32 Close = SourceText.Find(TEXT("}"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Open + 1);
		if (Close == INDEX_NONE)
		{
			break;
		}
		const FString Name = SourceText.Mid(Open + 1, Close - Open - 1);
		if (!Name.IsEmpty())
		{
			OutArgs.AddUnique(Name);
		}
		SearchFrom = Close + 1;
	}
	OutArgs.Sort();
}
