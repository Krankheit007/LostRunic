// Copyright LostRunic. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "UObject/Package.h"

namespace
{
	bool FindRemovedTemplateToken(const FString& value, const TArray<FString>& removedClassTokens, FString& outToken)
	{
		for (const FString& token : removedClassTokens)
		{
			if (value.Contains(token))
			{
				outToken = token;
				return true;
			}
		}
		return false;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRRemovedTemplateAssetReferences,
	"LostRunic.AssetRegistry.RemovedTemplateClassesHaveNoBlueprintParents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRRemovedTemplateAssetReferences::RunTest(const FString& Parameters)
{
	IAssetRegistry& assetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	TArray<FAssetData> assets;
	assetRegistry.GetAssetsByPath(FName(TEXT("/Game")), assets, true);

	const TArray<FString> removedClassTokens =
	{
		TEXT("LostRunicCharacter"),
		TEXT("LostRunicGameMode"),
		TEXT("LostRunicPlayerController"),
		TEXT("Variant_Strategy"),
		TEXT("Variant_TwinStick")
	};

	const TArray<FName> referenceTags =
	{
		FName(TEXT("NativeParentClass")),
		FName(TEXT("ParentClass")),
		FName(TEXT("BlueprintParentClass")),
		FName(TEXT("GeneratedClass")),
		FName(TEXT("ClassPath")),
		FName(TEXT("SoftObjectPath")),
		FName(TEXT("ObjectPath")),
		FName(TEXT("AssetClass"))
	};

	for (const FAssetData& asset : assets)
	{
		TArray<FString> references;
		references.Add(asset.GetObjectPathString());
		references.Add(asset.PackageName.ToString());
		references.Add(asset.PackagePath.ToString());
		for (const FName tagName : referenceTags)
		{
			const FAssetTagValueRef tagValue = asset.TagsAndValues.FindTag(tagName);
			if (tagValue.IsSet())
			{
				references.Add(tagValue.AsString());
			}
		}

		for (const FString& reference : references)
		{
			FString removedToken;
			if (FindRemovedTemplateToken(reference, removedClassTokens, removedToken))
			{
				AddError(FString::Printf(TEXT("Asset %s still references removed template token %s through %s."),
					*asset.GetObjectPathString(), *removedToken, *reference));
			}
		}
	}

	TArray<FString> configFiles;
	IFileManager::Get().FindFilesRecursive(configFiles, *FPaths::ProjectConfigDir(), TEXT("*.ini"), true, false);
	for (const FString& configFile : configFiles)
	{
		FString configText;
		if (!FFileHelper::LoadFileToString(configText, *configFile))
		{
			continue;
		}
		FString removedToken;
		if (FindRemovedTemplateToken(configText, removedClassTokens, removedToken))
		{
			AddError(FString::Printf(TEXT("Config file %s still references removed template token %s."),
				*configFile, *removedToken));
		}
	}

	TArray<FAssetData> pieAssets;
	assetRegistry.GetAssetsByPath(FName(TEXT("/Game/LostRunic/Levels/PIE_Test")), pieAssets, true);
	TestTrue(TEXT("PIE test package is present in the Asset Registry"), pieAssets.Num() > 0);
	UPackage* piePackage = LoadPackage(nullptr, TEXT("/Game/LostRunic/Levels/PIE_Test/L_PIE_Test"), LOAD_None);
	TestNotNull(TEXT("PIE test package loads without removed template references"), piePackage);

	return true;
}

#endif