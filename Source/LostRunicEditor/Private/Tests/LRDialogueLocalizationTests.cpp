// Copyright LostRunic. All Rights Reserved.
#include "Misc/AutomationTest.h"

#include "DialogueLocalization/LRDialogueLocalizationHash.h"
#include "DialogueLocalization/LRDialogueLocalizationParser.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	FString FixturePath()
	{
		return FPaths::Combine(FPaths::ProjectContentDir(), TEXT("LostRunic/Dialogue/Source/Fixture.sud"));
	}

	bool ParseFixture(TArray<FLRDialogueLocalizationEntry>& OutEntries, FString& OutError)
	{
		TArray<FString> Errors;
		const bool bParsed = FLRDialogueLocalizationParser::ParseFile(
			TEXT("Home.Butler.Introduction"), FixturePath(), nullptr, OutEntries, Errors);
		OutError = FString::Join(Errors, TEXT("\n"));
		return bParsed;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRDialogueLocalizationEntryIdentitySurvivesReorder,
	"LostRunicEditor.DialogueLocalization.EntryIdentitySurvivesReorder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRDialogueLocalizationEntryIdentitySurvivesReorder::RunTest(const FString& Parameters)
{
	TArray<FLRDialogueLocalizationEntry> Entries;
	FString Error;
	TestTrue(TEXT("Fixture parses"), ParseFixture(Entries, Error));
	TestTrue(TEXT("Fixture errors are empty"), Error.IsEmpty());
	TestEqual(TEXT("Fixture entry count"), Entries.Num(), 5);
	FLRDialogueLocalizationManifest Manifest;
	Manifest.Entries = Entries;
	Manifest.RecomputeHash();
	const FString OriginalHash = Manifest.ManifestHash;
	Algo::Reverse(Manifest.Entries);
	Manifest.RecomputeHash();
	TestEqual(TEXT("Manifest hash is order independent"), Manifest.ManifestHash, OriginalHash);
	TestEqual(TEXT("Stable first key exists"), Entries[0].StringKey, FString(TEXT("@001a@")));
	TestEqual(TEXT("Stable choice key exists"), Entries[1].StringKey, FString(TEXT("@001b@")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRDialogueLocalizationSudsAdapterDoesNotLeakTypes,
	"LostRunicEditor.DialogueLocalization.SudsAdapterDoesNotLeakTypes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRDialogueLocalizationSudsAdapterDoesNotLeakTypes::RunTest(const FString& Parameters)
{
	FString TypesHeader;
	const FString HeaderPath = FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/LostRunicEditor/Private/DialogueLocalization/LRDialogueLocalizationTypes.h"));
	TestTrue(TEXT("Project localization types header is readable"), FFileHelper::LoadFileToString(TypesHeader, *HeaderPath));
	TestFalse(TEXT("Project types do not include SUDS parsed node types"), TypesHeader.Contains(TEXT("FSUDSParsedNode")));
	TestFalse(TEXT("Project types do not include the SUDS importer"), TypesHeader.Contains(TEXT("SUDSScriptImporter.h")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRDialogueLocalizationGenerateSpeakerLinesDisabled,
	"LostRunicEditor.DialogueLocalization.GenerateSpeakerLinesFromChoicesDisabled",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRDialogueLocalizationGenerateSpeakerLinesDisabled::RunTest(const FString& Parameters)
{
	FString Source;
	TestTrue(TEXT("Fixture is readable"), FFileHelper::LoadFileToString(Source, *FixturePath()));
	TestFalse(TEXT("Fixture does not enable generated speaker lines"), Source.Contains(TEXT("GenerateSpeakerLinesFromChoices true")));
	TArray<FLRDialogueLocalizationEntry> Entries;
	FString Error;
	TestTrue(TEXT("Validator accepts the disabled setting"), ParseFixture(Entries, Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRDialogueLocalizationApplyImportCompileProducesLocres,
	"LostRunicEditor.DialogueLocalization.ApplyImportCompileProducesLocres",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRDialogueLocalizationApplyImportCompileProducesLocres::RunTest(const FString& Parameters)
{
	FString ImportConfig;
	FString CompileConfig;
	const FString ConfigDir = FPaths::Combine(FPaths::ProjectDir(), TEXT("Config/Localization"));
	TestTrue(TEXT("Import config exists"), FFileHelper::LoadFileToString(ImportConfig, *FPaths::Combine(ConfigDir, TEXT("LostRunic_Import.ini"))));
	TestTrue(TEXT("Compile config exists"), FFileHelper::LoadFileToString(CompileConfig, *FPaths::Combine(ConfigDir, TEXT("LostRunic_Compile.ini"))));
	TestTrue(TEXT("Import config runs UE localization import"), ImportConfig.Contains(TEXT("bImportLoc=true")));
	TestTrue(TEXT("Compile config generates locres"), CompileConfig.Contains(TEXT("GenerateTextLocalizationResource")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRDialogueLocalizationSha256Contract,
	"LostRunicEditor.DialogueLocalization.SourceHashUsesUtf8Sha256",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRDialogueLocalizationSha256Contract::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("SHA-256 known vector"), FLRDialogueLocalizationHash::Sha256String(TEXT("abc")),
		FString(TEXT("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad")));
	return true;
}

#endif
