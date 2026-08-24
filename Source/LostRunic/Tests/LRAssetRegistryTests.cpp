// Copyright LostRunic. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "AssetRegistry/AssetRegistryHelpers.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Modules/ModuleManager.h"

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

	for (const FAssetData& asset : assets)
	{
		FString parentClass;
		if (!UAssetRegistryHelpers::GetTagValue(asset, FName(TEXT("NativeParentClass")), parentClass)
			&& !UAssetRegistryHelpers::GetTagValue(asset, FName(TEXT("ParentClass")), parentClass))
		{
			continue;
		}

		for (const FString& token : removedClassTokens)
		{
			if (parentClass.Contains(token))
			{
				AddError(FString::Printf(TEXT("Asset %s still has removed parent class %s."),
					*asset.GetObjectPathString(), *parentClass));
			}
		}
	}

	return true;
}

#endif