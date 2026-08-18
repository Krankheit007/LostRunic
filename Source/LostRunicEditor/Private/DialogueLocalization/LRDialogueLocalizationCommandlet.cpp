// Copyright LostRunic. All Rights Reserved.
#include "DialogueLocalization/LRDialogueLocalizationCommandlet.h"

#include "DialogueLocalization/LRDialogueLocalizationPO.h"
#include "DialogueLocalization/LRDialogueLocalizationParser.h"
#include "DialogueLocalization/LRDialogueLocalizationSettings.h"
#include "DialogueLocalization/LRDialogueLocalizationLog.h"
#include "DialogueLocalization/LRDialogueLocalizationTypes.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "EditorReimportHandler.h"
#include "Narrative/LRDialogueScriptRegistry.h"
#include "Narrative/LRDialogueSpeakerRegistry.h"
#include "SUDSScript.h"
#include "Internationalization/Text.h"

namespace
{
	FString GetParam(const FString& Params, const TCHAR* Name, const FString& DefaultValue = FString())
	{
		FString Value;
		return FParse::Value(*Params, Name, Value) ? Value : DefaultValue;
	}

	FString ResolveProjectPath(const FString& Path)
	{
		return FPaths::IsRelative(Path) ? FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), Path) : Path;
	}

	void AddErrorsToLog(const TArray<FString>& Errors)
	{
		for (const FString& Error : Errors)
		{
			UE_LOG(LogLostRunicDialogueLocalization, Error, TEXT("%s"), *Error);
		}
	}

	bool RunProcess(const FString& Executable, const FString& Arguments)
	{
		uint32 ProcessId = 0;
		FProcHandle Handle = FPlatformProcess::CreateProc(*Executable, *Arguments, false, false, false, &ProcessId, 0, nullptr, nullptr);
		if (!Handle.IsValid())
		{
			UE_LOG(LogLostRunicDialogueLocalization, Error, TEXT("Failed to launch: %s %s"), *Executable, *Arguments);
			return false;
		}
		FPlatformProcess::WaitForProc(Handle);
		int32 ReturnCode = INDEX_NONE;
		FPlatformProcess::GetProcReturnCode(Handle, &ReturnCode);
		FPlatformProcess::CloseProc(Handle);
		return ReturnCode == 0;
	}

	bool FindOrLoadScript(const FString& RegistryPath, const FString& ScriptId, USUDSScript*& OutScript)
	{
		OutScript = nullptr;
		if (RegistryPath.IsEmpty())
		{
			return true;
		}
		ULRDialogueScriptRegistry* Registry = LoadObject<ULRDialogueScriptRegistry>(nullptr, *RegistryPath);
		if (!Registry)
		{
			UE_LOG(LogLostRunicDialogueLocalization, Error, TEXT("Unable to load Registry asset %s."), *RegistryPath);
			return false;
		}
		TObjectPtr<USUDSScript> Script;
		FString Error;
		if (!Registry->Resolve(FName(*ScriptId), Script, Error))
		{
			UE_LOG(LogLostRunicDialogueLocalization, Error, TEXT("%s"), *Error);
			return false;
		}
		OutScript = Script.Get();
		return true;
	}

	bool ReimportRegistryScripts(const FString& RegistryPath)
	{
		ULRDialogueScriptRegistry* Registry = LoadObject<ULRDialogueScriptRegistry>(nullptr, *RegistryPath);
		if (!Registry)
		{
			UE_LOG(LogLostRunicDialogueLocalization, Error, TEXT("Unable to load Registry asset for SUDS reimport: %s."), *RegistryPath);
			return false;
		}
		for (const FLRDialogueScriptDefinition& Definition : Registry->Scripts)
		{
			if (!Definition.Script)
			{
				UE_LOG(LogLostRunicDialogueLocalization, Error, TEXT("Registry contains an empty Script for ScriptId=%s."), *Definition.ScriptId.ToString());
				return false;
			}
			TArray<FString> SourceFiles;
			if (!FReimportManager::Instance()->CanReimport(Definition.Script, &SourceFiles) || SourceFiles.IsEmpty())
			{
				UE_LOG(LogLostRunicDialogueLocalization, Error, TEXT("SUDS asset %s has no reimport source."), *Definition.Script->GetPathName());
				return false;
			}
			if (!FReimportManager::Instance()->Reimport(Definition.Script, false, false, TEXT(""), nullptr,
				INDEX_NONE, false, true))
			{
				UE_LOG(LogLostRunicDialogueLocalization, Error, TEXT("SUDS reimport failed for ScriptId=%s."), *Definition.ScriptId.ToString());
				return false;
			}
		}
		return true;
	}

	bool BuildManifest(const FString& ScriptId, const FString& ScriptPath, const FString& RegistryPath,
		const FString& SpeakerRegistryPath, FString& OutManifestPath)
	{
		USUDSScript* ImportedScript = nullptr;
		if (!FindOrLoadScript(RegistryPath, ScriptId, ImportedScript))
		{
			return false;
		}
		TArray<FLRDialogueLocalizationEntry> Entries;
		TArray<FString> Errors;
		if (!FLRDialogueLocalizationParser::ParseFile(ScriptId, ScriptPath, ImportedScript, Entries, Errors))
		{
			AddErrorsToLog(Errors);
			return false;
		}
		FLRDialogueLocalizationManifest Manifest;
		Manifest.Entries = MoveTemp(Entries);
		Manifest.SourceRevision = FString::Printf(TEXT("SUDS:%s"), *FPaths::GetCleanFilename(ScriptPath));
		ULRDialogueSpeakerRegistry* SpeakerRegistry = nullptr;
		if (!SpeakerRegistryPath.IsEmpty())
		{
			SpeakerRegistry = LoadObject<ULRDialogueSpeakerRegistry>(nullptr, *SpeakerRegistryPath);
			if (!SpeakerRegistry)
			{
				UE_LOG(LogLostRunicDialogueLocalization, Error, TEXT("Unable to load Speaker Registry asset %s."), *SpeakerRegistryPath);
				return false;
			}
			FString SpeakerError;
			if (!SpeakerRegistry->Validate(SpeakerError))
			{
				UE_LOG(LogLostRunicDialogueLocalization, Error, TEXT("%s"), *SpeakerError);
				return false;
			}
		}
		for (const FLRDialogueLocalizationEntry& Entry : Manifest.Entries)
		{
			if (!Entry.SpeakerId.IsEmpty() && !Manifest.Speakers.ContainsByPredicate([&Entry](const FLRDialogueLocalizationSpeaker& Speaker)
			{
				return Speaker.SpeakerId == Entry.SpeakerId;
			}))
			{
				FLRDialogueLocalizationSpeaker& Speaker = Manifest.Speakers.AddDefaulted_GetRef();
				Speaker.SpeakerId = Entry.SpeakerId;
				if (SpeakerRegistry)
				{
					if (const FLRDialogueSpeakerDefinition* Definition = SpeakerRegistry->Find(FName(*Entry.SpeakerId)))
					{
						Speaker.DisplayName = Definition->DisplayName.ToString();
						Speaker.SourceText = Speaker.DisplayName;
						Speaker.SourceHash = FLRDialogueLocalizationParser::ComputeSourceHash(Speaker.SourceText);
						FName TableId;
						FString TableKey;
						FTextInspector::GetTableIdAndKey(Definition->DisplayName, TableId, TableKey);
						Speaker.TextTableId = TableId.ToString();
						Speaker.TextTableKey = TableKey;
						if (const TOptional<FString> Namespace = FTextInspector::GetNamespace(Definition->DisplayName))
						{
							Speaker.LocNamespace = Namespace.GetValue();
						}
						if (const TOptional<FString> Key = FTextInspector::GetKey(Definition->DisplayName))
						{
							Speaker.LocKey = Key.GetValue();
						}
					}
					else
					{
						UE_LOG(LogLostRunicDialogueLocalization, Error, TEXT("SpeakerId=%s is not present in the Speaker Registry."), *Entry.SpeakerId);
						return false;
					}
				}
			}
		}
		OutManifestPath = FPaths::Combine(FPaths::ProjectIntermediateDir(), TEXT("DialogueLocalization"), TEXT("DialogueLocalizationManifest.json"));
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(OutManifestPath), true);
		FString Error;
		if (!Manifest.Save(OutManifestPath, Error))
		{
			UE_LOG(LogLostRunicDialogueLocalization, Error, TEXT("%s"), *Error);
			return false;
		}
		UE_LOG(LogLostRunicDialogueLocalization, Display, TEXT("Manifest written: %s (%d entries)"), *OutManifestPath, Manifest.Entries.Num());
		return true;
	}

	bool RunPython(const FString& ScriptName, const FString& Arguments, const FString& Params)
	{
		FString PythonOverride = GetParam(Params, TEXT("PythonExecutable="));
		if (PythonOverride.IsEmpty())
		{
			PythonOverride = ULRDialogueLocalizationSettings::Get()->PythonExecutableOverride.FilePath;
		}
		const FString PythonExecutable = PythonOverride.IsEmpty()
			? FPaths::Combine(FPaths::ProjectIntermediateDir(), TEXT("DialogueLocalization/PythonEnv/Scripts/python.exe"))
			: ResolveProjectPath(PythonOverride);
		const FString ReadyFile = FPaths::Combine(FPaths::GetPath(PythonExecutable), TEXT("../.ready"));
		if (!FPaths::FileExists(PythonExecutable) || !FPaths::FileExists(ReadyFile))
		{
			UE_LOG(LogLostRunicDialogueLocalization, Error, TEXT("Python environment is not ready. Run Tools/DialogueLocalization/bootstrap.ps1 or pass -PythonExecutable."));
			return false;
		}
		return RunProcess(PythonExecutable, FString::Printf(TEXT("\"%s\" %s"), *ResolveProjectPath(ScriptName), *Arguments));
	}

	bool RunGatherTextConfig(const FString& ConfigPath)
	{
		const FString EditorExecutable = FPlatformProcess::ExecutablePath();
		return RunProcess(EditorExecutable, FString::Printf(TEXT("-project=\"%s\" -run=GatherText -config=\"%s\" -unattended -nop4"),
			*FPaths::GetProjectFilePath(), *ResolveProjectPath(ConfigPath)));
	}
}

ULRDialogueLocalizationCommandlet::ULRDialogueLocalizationCommandlet()
{
	IsClient = false;
	IsServer = false;
	LogToConsole = true;
	ShowErrorCount = true;
}

int32 ULRDialogueLocalizationCommandlet::Main(const FString& Params)
{
	const FString Mode = GetParam(Params, TEXT("Mode="), TEXT("ValidateSUDS"));
	const FString ScriptId = GetParam(Params, TEXT("ScriptId="), TEXT("Home.Butler.Introduction"));
	const FString ScriptPath = ResolveProjectPath(GetParam(Params, TEXT("Script="), TEXT("Content/LostRunic/Dialogue/Source/Fixture.sud")));
	const FString RegistryPath = GetParam(Params, TEXT("Registry="));
	const FString SpeakerRegistryPath = GetParam(Params, TEXT("SpeakerRegistry="));
	const FString Culture = GetParam(Params, TEXT("Culture="), TEXT("en"));
	const FString ManifestPath = ResolveProjectPath(GetParam(Params, TEXT("Manifest="), TEXT("Intermediate/DialogueLocalization/DialogueLocalizationManifest.json")));
	const FString POPath = ResolveProjectPath(GetParam(Params, TEXT("PO=")));
	const FString ImportPath = ResolveProjectPath(GetParam(Params, TEXT("Import="), TEXT("Intermediate/DialogueLocalization/DialogueLocalizationImport.json")));
	const bool bStrictManifest = FParse::Param(*Params, TEXT("StrictManifest"));

	if (Mode.Equals(TEXT("ValidateSUDS"), ESearchCase::IgnoreCase) || Mode.Equals(TEXT("PrepareXLSX"), ESearchCase::IgnoreCase))
	{
		if (Mode.Equals(TEXT("PrepareXLSX"), ESearchCase::IgnoreCase))
		{
			if (RegistryPath.IsEmpty() || SpeakerRegistryPath.IsEmpty() || !ReimportRegistryScripts(RegistryPath))
			{
				UE_LOG(LogLostRunicDialogueLocalization, Error, TEXT("PrepareXLSX requires valid -Registry= and -SpeakerRegistry= assets with reimportable SUDS scripts."));
				return 1;
			}
			if (!RunGatherTextConfig(GetParam(Params, TEXT("GatherConfig="), TEXT("Config/Localization/LostRunic_Gather.ini")))
				|| !RunGatherTextConfig(GetParam(Params, TEXT("ExportConfig="), TEXT("Config/Localization/LostRunic_Export.ini"))))
			{
				return 1;
			}
		}
		FString GeneratedManifestPath;
		if (!BuildManifest(ScriptId, ScriptPath, RegistryPath, SpeakerRegistryPath, GeneratedManifestPath))
		{
			return 1;
		}
		if (Mode.Equals(TEXT("ValidateSUDS"), ESearchCase::IgnoreCase))
		{
			return 0;
		}
		if (POPath.IsEmpty())
		{
			UE_LOG(LogLostRunicDialogueLocalization, Error, TEXT("PrepareXLSX requires -PO=... from the current UE Export PO step."));
			return 1;
		}
		const FString RowsPath = FPaths::Combine(FPaths::ProjectIntermediateDir(), TEXT("DialogueLocalization/DialogueWorkbookRows.json"));
		FString Error;
		if (!FLRDialogueLocalizationPO::BuildWorkbookRows(GeneratedManifestPath, POPath, Culture, RowsPath, Error))
		{
			UE_LOG(LogLostRunicDialogueLocalization, Error, TEXT("%s"), *Error);
			return 1;
		}
		const FString WorkbookPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("DialogueLocalization/Workbooks"), FString::Printf(TEXT("Dialogue_%s.xlsx"), *Culture));
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(WorkbookPath), true);
		const FString PythonArgs = FString::Printf(TEXT("--input \"%s\" --output \"%s\" --culture \"%s\""), *RowsPath, *WorkbookPath, *Culture);
		return RunPython(TEXT("Tools/DialogueLocalization/export_xlsx.py"), PythonArgs, Params) ? 0 : 1;
	}
	if (Mode.Equals(TEXT("ImportXLSX"), ESearchCase::IgnoreCase))
	{
		const FString WorkbookPath = ResolveProjectPath(GetParam(Params, TEXT("XLSX=")));
		const FString PythonArgs = FString::Printf(TEXT("--input \"%s\" --output \"%s\" --culture \"%s\""), *WorkbookPath, *ImportPath, *Culture);
		return RunPython(TEXT("Tools/DialogueLocalization/import_xlsx.py"), PythonArgs, Params) ? 0 : 1;
	}
	if (Mode.Equals(TEXT("ApplyPO"), ESearchCase::IgnoreCase))
	{
		FString Error;
		if (FLRDialogueLocalizationPO::ApplyImportJSON(ManifestPath, POPath, ImportPath, bStrictManifest, Error))
		{
			return 0;
		}
		UE_LOG(LogLostRunicDialogueLocalization, Error, TEXT("%s"), *Error);
		return 1;
	}
	if (Mode.Equals(TEXT("ImportLocalization"), ESearchCase::IgnoreCase))
	{
		return RunGatherTextConfig(GetParam(Params, TEXT("Config="), TEXT("Config/Localization/LostRunic_Import.ini"))) ? 0 : 1;
	}
	if (Mode.Equals(TEXT("CompileLocalization"), ESearchCase::IgnoreCase))
	{
		return RunGatherTextConfig(GetParam(Params, TEXT("Config="), TEXT("Config/Localization/LostRunic_Compile.ini"))) ? 0 : 1;
	}
	if (Mode.Equals(TEXT("ApplyAndCompileLocalization"), ESearchCase::IgnoreCase))
	{
		FString Error;
		if (!FLRDialogueLocalizationPO::ApplyImportJSON(ManifestPath, POPath, ImportPath, bStrictManifest, Error))
		{
			UE_LOG(LogLostRunicDialogueLocalization, Error, TEXT("%s"), *Error);
			return 1;
		}
		if (!RunGatherTextConfig(GetParam(Params, TEXT("ImportConfig="), TEXT("Config/Localization/LostRunic_Import.ini"))))
		{
			return 1;
		}
		return RunGatherTextConfig(GetParam(Params, TEXT("CompileConfig="), TEXT("Config/Localization/LostRunic_Compile.ini"))) ? 0 : 1;
	}

	UE_LOG(LogLostRunicDialogueLocalization, Error, TEXT("Unknown Mode=%s."), *Mode);
	return 1;
}
