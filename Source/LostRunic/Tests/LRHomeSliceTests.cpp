#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Data/LRGameContentSet.h"
#include "Data/LRItemDefinition.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "UI/LRScreenWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRHomeContentContractTest, "LostRunic.Home.ContentContractIsComplete",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLRHomeContentContractTest::RunTest(const FString& parameters)
{
	const ULRGameContentSet* contentSet = LoadObject<ULRGameContentSet>(
		nullptr, TEXT("/Game/LostRunic/Data/DA_LRGameContentSet.DA_LRGameContentSet"));
	if (!TestNotNull(TEXT("Content set loads"), contentSet))
	{
		return false;
	}
	FString validationError;
	TestTrue(*FString::Printf(TEXT("Content validates: %s"), *validationError), contentSet->Validate(validationError));
	TestNotNull(TEXT("Home map registered"), contentSet->FindMap(TEXT("Home")).LoadSynchronous());
	TestNotNull(TEXT("Memory map registered"), contentSet->FindMap(TEXT("Memory")).LoadSynchronous());
	TestTrue(TEXT("Home key definition is registered"), contentSet->Items.ContainsByPredicate([](const ULRItemDefinition* item)
	{
		return item && item->ItemId == TEXT("Home.Key");
	}));
	TestTrue(TEXT("Courage definition is registered"), contentSet->Items.ContainsByPredicate([](const ULRItemDefinition* item)
	{
		return item && item->ItemId == TEXT("Home.CourageCharm");
	}));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRHomeAssetLoadTest, "LostRunic.Home.BlueprintsAndScreensLoad",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLRHomeAssetLoadTest::RunTest(const FString& parameters)
{
	const TCHAR* classPaths[] = {
		TEXT("/Game/LostRunic/Blueprints/Framework/BP_LRGameMode.BP_LRGameMode_C"),
		TEXT("/Game/LostRunic/Blueprints/Characters/BP_LRCharacter.BP_LRCharacter_C"),
		TEXT("/Game/LostRunic/Blueprints/Characters/BP_LRGuard.BP_LRGuard_C"),
		TEXT("/Game/LostRunic/UI/WBP_LRHUD.WBP_LRHUD_C"),
		TEXT("/Game/LostRunic/UI/WBP_LRNarrative.WBP_LRNarrative_C"),
		TEXT("/Game/LostRunic/UI/WBP_LRTransition.WBP_LRTransition_C")
	};
	for (const TCHAR* classPath : classPaths)
	{
		TestNotNull(classPath, LoadObject<UClass>(nullptr, classPath));
	}
	TestNotNull(TEXT("Home world loads"), LoadObject<UWorld>(
		nullptr, TEXT("/Game/LostRunic/Levels/Home/L_Home.L_Home")));
	TestNotNull(TEXT("Memory world loads"), LoadObject<UWorld>(
		nullptr, TEXT("/Game/LostRunic/Levels/Memory/L_Memory.L_Memory")));
	return true;
}

#endif
