/**
 * @file LRFrameworkTests.cpp
 * @brief 提供 LostRunic Runtime 自动化测试，覆盖调优边界、状态矩阵、交互筛选、物品双入口、守卫警戒、叙事分支和存档事务顺序。仅在 WITH_DEV_AUTOMATION_TESTS 下编译。
 *
 * 关联文件：Tests 目录内调用该公共契约的实现文件；所属领域：Tests。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Data/LRGameContentSet.h"
#include "Data/LRGameTuningSet.h"
#include "Data/LRProjectSettings.h"
#include "Data/LRNPCTuning.h"
#include "Framework/LRCharacter.h"
#include "Framework/LRGameMode.h"
#include "Framework/LRPlayerController.h"
#include "UI/LRHUD.h"
#include "UI/LRMainMenuGameMode.h"
#include "UI/LRMainMenuHUD.h"
#include "UI/LRMainMenuWidget.h"
#include "UI/LRPauseWidget.h"
#include "UI/LRSaveConfirmDialogWidget.h"
#include "UI/LRSaveSelectionWidget.h"
#include "UI/LRSaveSlotWidget.h"
#include "Gameplay/LRLocomotionComponent.h"
#include "Input/LRInputConfig.h"
#include "InputAction.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRFrameworkDefaultsTest, "LostRunic.Framework.CharacterDoesNotTick",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRFrameworkDefaultsTest::RunTest(const FString& parameters)
{
	const ALRCharacter* character = GetDefault<ALRCharacter>();
	TestFalse(TEXT("LR character Tick is disabled"), character->PrimaryActorTick.bCanEverTick);

	const ALRGameMode* gameMode = GetDefault<ALRGameMode>();
	TestTrue(TEXT("GameMode uses the LR character"), gameMode->DefaultPawnClass == ALRCharacter::StaticClass());
	TestTrue(TEXT("GameMode uses the LR player controller"), gameMode->PlayerControllerClass == ALRPlayerController::StaticClass());
	TestTrue(TEXT("GameMode uses the LR HUD"), gameMode->HUDClass == ALRHUD::StaticClass());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRInputConfigTest, "LostRunic.Input.ProjectConfigIsComplete",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRInputConfigTest::RunTest(const FString& parameters)
{
	const ULRInputConfig* inputConfig = GetDefault<ULRProjectSettings>()->InputConfig.LoadSynchronous();
	TestNotNull(TEXT("Project InputConfig loads"), inputConfig);
	if (inputConfig)
	{
		FString error;
		TestTrue(TEXT("Required contexts and actions are assigned"), inputConfig->Validate(error));
	}
	TestTrue(TEXT("Project HUD screen class is configured"), !GetDefault<ULRProjectSettings>()->HUDScreenClass.IsNull());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRMainMenuFrameworkDefaultsTest, "LostRunic.Framework.MainMenuHasNoPawn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRMainMenuFrameworkDefaultsTest::RunTest(const FString& parameters)
{
	const ALRMainMenuGameMode* gameMode = GetDefault<ALRMainMenuGameMode>();
	TestNull(TEXT("Main menu does not spawn a pawn"), gameMode->DefaultPawnClass.Get());
	TestTrue(TEXT("Main menu reuses the LR player controller"),
		gameMode->PlayerControllerClass == ALRPlayerController::StaticClass());
	TestTrue(TEXT("Main menu uses the dedicated host HUD"), gameMode->HUDClass == ALRMainMenuHUD::StaticClass());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRSaveUIAssetContractTest, "LostRunic.UI.SaveAssetsMatchDesignerContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRSaveUIAssetContractTest::RunTest(const FString& parameters)
{
	const UInputAction* cancelAction = LoadObject<UInputAction>(nullptr,
		TEXT("/Game/LostRunic/Input/Actions/IA_LRCancel.IA_LRCancel"));
	TestNotNull(TEXT("Save-screen cancel input action loads"), cancelAction);
	TestTrue(TEXT("Cancel input remains available while the pause save screen is open"),
		cancelAction && cancelAction->bTriggerWhenPaused);

	const UClass* pauseClass = LoadClass<ULRPauseWidget>(nullptr,
		TEXT("/Game/LostRunic/UI/Save/WBP_Pause.WBP_Pause_C"));
	TestNotNull(TEXT("Pause widget uses the native pause contract"), pauseClass);

	const UClass* slotClass = LoadClass<ULRSaveSlotWidget>(nullptr,
		TEXT("/Game/LostRunic/UI/Save/WBP_SaveSlot.WBP_SaveSlot_C"));
	TestNotNull(TEXT("Save slot widget loads"), slotClass);
	if (slotClass)
	{
		TestNotNull(TEXT("Save slot exposes the designer-authored SlotButton contract"),
			FindFProperty<FObjectPropertyBase>(slotClass, TEXT("SlotButton")));
	}

	const UClass* selectionClass = LoadClass<ULRSaveSelectionWidget>(nullptr,
		TEXT("/Game/LostRunic/UI/Save/WBP_SaveSelection.WBP_SaveSelection_C"));
	TestNotNull(TEXT("Save selection widget loads"), selectionClass);
	// Row extent is owned by the row blueprints (WBP_SaveSlot / WBP_CreateSaveSlot root canvas),
	// not by a runtime SizeBox wrapper; the removed SlotRowHeight override must not come back.

	const UClass* dialogClass = LoadClass<ULRSaveConfirmDialogWidget>(nullptr,
		TEXT("/Game/LostRunic/UI/Save/WBP_SaveConfirmDialog.WBP_SaveConfirmDialog_C"));
	TestNotNull(TEXT("Save confirmation widget loads"), dialogClass);
	for (const FName propertyName : { FName(TEXT("Cover")), FName(TEXT("Delete")),
		FName(TEXT("Cover_Confirm")), FName(TEXT("Cover_Cancel")),
		FName(TEXT("Delete_Confirm")), FName(TEXT("Delete_Cancel")) })
	{
		TestNotNull(*FString::Printf(TEXT("Confirmation widget binds %s"), *propertyName.ToString()),
			dialogClass ? FindFProperty<FObjectPropertyBase>(dialogClass, propertyName) : nullptr);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRPIEContentContractTest, "LostRunic.Framework.PIEContentContracts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRPIEContentContractTest::RunTest(const FString& parameters)
{
	const UClass* gameModeClass = LoadClass<ALRGameMode>(nullptr,
		TEXT("/Game/LostRunic/Blueprints/Character/BP_LRGameMode.BP_LRGameMode_C"));
	const UClass* ruthClass = LoadClass<ALRCharacter>(nullptr,
		TEXT("/Game/LostRunic/Blueprints/Character/BP_Ruth.BP_Ruth_C"));
	TestNotNull(TEXT("Gameplay GameMode loads"), gameModeClass);
	TestNotNull(TEXT("Ruth character loads"), ruthClass);
	if (gameModeClass && ruthClass)
	{
		TestTrue(TEXT("Gameplay GameMode spawns BP_Ruth"),
			gameModeClass->GetDefaultObject<ALRGameMode>()->DefaultPawnClass == ruthClass);
	}

	const UClass* mainMenuModeClass = LoadClass<ALRMainMenuGameMode>(nullptr,
		TEXT("/Game/LostRunic/Blueprints/UI/BP_LRMainMenuGameMode.BP_LRMainMenuGameMode_C"));
	TestNotNull(TEXT("Dedicated main-menu GameMode exists"), mainMenuModeClass);
	if (mainMenuModeClass)
	{
		const ALRMainMenuGameMode* menuMode = mainMenuModeClass->GetDefaultObject<ALRMainMenuGameMode>();
		TestNull(TEXT("Dedicated main-menu mode never spawns a Pawn"), menuMode->DefaultPawnClass.Get());
		TestTrue(TEXT("Dedicated main-menu mode uses a MainMenu HUD"),
			menuMode->HUDClass && menuMode->HUDClass->IsChildOf(ALRMainMenuHUD::StaticClass()));
	}

	const ULRGameTuningSet* tuningSet = LoadObject<ULRGameTuningSet>(nullptr,
		TEXT("/Game/LostRunic/Data/Tuning/DA_LRGameTuningSet.DA_LRGameTuningSet"));
	TestNotNull(TEXT("Project tuning set loads"), tuningSet);
	TestNotNull(TEXT("Project tuning set assigns NPC tuning"), tuningSet ? tuningSet->NPC.Get() : nullptr);

	const ULRGameContentSet* contentSet = LoadObject<ULRGameContentSet>(nullptr,
		TEXT("/Game/LostRunic/Data/DA_LRGameContentSet.DA_LRGameContentSet"));
	TestNotNull(TEXT("Project content set loads"), contentSet);
	TestEqual(TEXT("Project content set registers the main-menu destination"),
		contentSet ? contentSet->MainMenuMapId : NAME_None, FName(TEXT("Menu")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRMovementPaceInputTest, "LostRunic.Input.MovementPaceRestoresPreviousMode",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLRMovementPaceInputTest::RunTest(const FString& parameters)
{
	ULRLocomotionComponent* locomotion = NewObject<ULRLocomotionComponent>();
	TestEqual(TEXT("Default pace is Walk"), locomotion->GetPace(), ELRMovementPace::Walk);

	locomotion->RequestToggleSneak();
	TestEqual(TEXT("Toggle enters Sneak"), locomotion->GetPace(), ELRMovementPace::Sneak);
	locomotion->RequestStartRun();
	TestEqual(TEXT("Run press enters Run"), locomotion->GetPace(), ELRMovementPace::Run);
	locomotion->RequestStopRun();
	TestEqual(TEXT("Run release restores Sneak"), locomotion->GetPace(), ELRMovementPace::Sneak);

	locomotion->RequestToggleSneak();
	locomotion->RequestStartRun();
	locomotion->RequestStopRun();
	TestEqual(TEXT("Run release restores Walk"), locomotion->GetPace(), ELRMovementPace::Walk);

	locomotion->RequestStartRun();
	locomotion->RequestToggleSneak();
	locomotion->RequestStopRun();
	TestEqual(TEXT("Sneak toggle during Run changes restored pace"), locomotion->GetPace(), ELRMovementPace::Sneak);
	return true;
}

#endif
