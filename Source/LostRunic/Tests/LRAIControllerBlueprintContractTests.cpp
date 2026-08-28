/**
 * @file LRAIControllerBlueprintContractTests.cpp
 * @brief Guards the Blueprint-only configuration contract for native AI Controller components.
 */
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AI/LRGuardAIController.h"
#include "AI/LRGuardCharacter.h"
#include "AI/LRNPCCharacter.h"
#include "AI/LRNPCController.h"
#include "Components/ActorComponent.h"
#include "Components/StateTreeAIComponent.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/Engine.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/World.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "StateTree.h"
#include "StateTreeReference.h"
#include "UObject/UnrealType.h"

namespace
{
	bool HasForbiddenAIComponentNode(const UBlueprint* blueprint)
	{
		const UBlueprintGeneratedClass* generatedClass = blueprint
			? Cast<UBlueprintGeneratedClass>(blueprint->GeneratedClass) : nullptr;
		const USimpleConstructionScript* scs = blueprint ? blueprint->SimpleConstructionScript : nullptr;
		if (!generatedClass || !scs)
		{
			return false;
		}
		for (const USCS_Node* node : scs->GetAllNodes())
		{
			const UActorComponent* component = node ? node->GetActualComponentTemplate(
				const_cast<UBlueprintGeneratedClass*>(generatedClass)) : nullptr;
			if (component && (component->IsA<UAIPerceptionComponent>()
				|| component->IsA<UStateTreeAIComponent>()))
			{
				return true;
			}
		}
		return false;
	}

	bool ReadBoolProperty(const UObject* object, const TCHAR* propertyName, bool& outValue)
	{
		const FBoolProperty* property = object
			? FindFProperty<FBoolProperty>(object->GetClass(), propertyName) : nullptr;
		if (!property)
		{
			return false;
		}
		outValue = property->GetPropertyValue_InContainer(object);
		return true;
	}

	const UStateTree* ReadConfiguredStateTree(const UStateTreeAIComponent* component)
	{
		const FStructProperty* property = component
			? FindFProperty<FStructProperty>(component->GetClass(), TEXT("StateTreeRef")) : nullptr;
		if (!property || property->Struct != FStateTreeReference::StaticStruct())
		{
			return nullptr;
		}
		const FStateTreeReference* reference =
			property->ContainerPtrToValuePtr<FStateTreeReference>(component);
		return reference ? reference->GetStateTree() : nullptr;
	}

	UWorld* CreateContractWorld()
	{
		const FName worldName = MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass(),
			TEXT("AIControllerBlueprintContractWorld"));
		UWorld* world = UWorld::CreateWorld(EWorldType::Game, false, worldName, GetTransientPackage());
		if (world && GEngine)
		{
			GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(world);
		}
		return world;
	}

	void DestroyContractWorld(UWorld* world)
	{
		if (!world)
		{
			return;
		}
		world->EndPlay(EEndPlayReason::Quit);
		if (GEngine)
		{
			GEngine->DestroyWorldContext(world);
		}
		world->DestroyWorld(false);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRGuardControllerBlueprintContractTest,
	"LostRunic.AI.GuardControllerBlueprintContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLRGuardControllerBlueprintContractTest::RunTest(const FString& parameters)
{
	UBlueprint* controllerBlueprint = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/LostRunic/Blueprints/Guard/BP_LRGuardController.BP_LRGuardController"));
	UClass* controllerClass = LoadClass<ALRGuardAIController>(nullptr,
		TEXT("/Game/LostRunic/Blueprints/Guard/BP_LRGuardController.BP_LRGuardController_C"));
	UClass* pawnClass = LoadClass<ALRGuardCharacter>(nullptr,
		TEXT("/Game/LostRunic/Blueprints/Guard/BP_Guard.BP_Guard_C"));
	if (!TestNotNull(TEXT("Guard Controller Blueprint loads"), controllerBlueprint)
		|| !TestNotNull(TEXT("Guard Controller generated class loads"), controllerClass)
		|| !TestNotNull(TEXT("Guard Pawn class loads"), pawnClass))
	{
		return false;
	}
	TestTrue(TEXT("Guard Controller parent is ALRGuardAIController"),
		controllerClass->GetSuperClass() == ALRGuardAIController::StaticClass());
	TestFalse(TEXT("Guard Controller SCS adds no AI components"),
		HasForbiddenAIComponentNode(controllerBlueprint));

	UWorld* world = CreateContractWorld();
	FActorSpawnParameters spawnParameters;
	spawnParameters.ObjectFlags = RF_Transient;
	ALRGuardAIController* controller = world
		? world->SpawnActor<ALRGuardAIController>(controllerClass, FTransform::Identity, spawnParameters) : nullptr;
	ALRGuardCharacter* pawn = world
		? world->SpawnActor<ALRGuardCharacter>(pawnClass, FTransform::Identity, spawnParameters) : nullptr;
	if (!TestNotNull(TEXT("Guard Controller instance spawns"), controller)
		|| !TestNotNull(TEXT("Guard Pawn instance spawns"), pawn))
	{
		DestroyContractWorld(world);
		return false;
	}

	TArray<UAIPerceptionComponent*> perceptionComponents;
	TArray<UStateTreeAIComponent*> stateTreeComponents;
	controller->GetComponents(perceptionComponents);
	controller->GetComponents(stateTreeComponents);
	TestEqual(TEXT("Guard Controller has one AIPerception"), perceptionComponents.Num(), 1);
	TestEqual(TEXT("Guard Controller has one StateTreeAI"), stateTreeComponents.Num(), 1);
	TestFalse(TEXT("Guard Pawn has no AIPerception"), pawn->FindComponentByClass<UAIPerceptionComponent>() != nullptr);
	TestFalse(TEXT("Guard Pawn has no StateTreeAI"), pawn->FindComponentByClass<UStateTreeAIComponent>() != nullptr);
	TestTrue(TEXT("Guard Pawn uses BP_LRGuardController"), pawn->AIControllerClass == controllerClass);

	if (perceptionComponents.Num() == 1 && stateTreeComponents.Num() == 1)
	{
		const UAISenseConfig_Sight* sight = perceptionComponents[0]->GetSenseConfig<UAISenseConfig_Sight>();
		const UAISenseConfig_Hearing* hearing = perceptionComponents[0]->GetSenseConfig<UAISenseConfig_Hearing>();
		TestNotNull(TEXT("Guard Sight is configured"), sight);
		TestNotNull(TEXT("Guard Hearing is configured"), hearing);
		const UClass* dominantSense = perceptionComponents[0]->GetDominantSense();
		TestTrue(*FString::Printf(TEXT("Guard dominant sense is Sight; actual=%s"),
			*GetNameSafe(dominantSense)), dominantSense == UAISense_Sight::StaticClass());
		if (sight)
		{
			TestEqual(TEXT("Guard Sight Radius"), sight->SightRadius, 500.0f);
			TestEqual(TEXT("Guard Lose Sight Radius"), sight->LoseSightRadius, 600.0f);
			TestTrue(*FString::Printf(TEXT("Guard Sight Half Angle is Blueprint-configured; actual=%.2f"),
				sight->PeripheralVisionAngleDegrees),
				sight->PeripheralVisionAngleDegrees > 0.0f
				&& sight->PeripheralVisionAngleDegrees <= 180.0f);
			TestEqual(TEXT("Guard Sight MaxAge is Never"), sight->GetMaxAge(),
				FAIStimulus::NeverHappenedAge);
			TestEqual(TEXT("Guard Auto Success range remains disabled"),
				sight->AutoSuccessRangeFromLastSeenLocation, -1.0f);
			TestTrue(TEXT("Guard Sight detects enemies"), sight->DetectionByAffiliation.bDetectEnemies);
			TestTrue(TEXT("Guard Sight detects friendlies"), sight->DetectionByAffiliation.bDetectFriendlies);
			TestTrue(TEXT("Guard Sight detects neutrals"), sight->DetectionByAffiliation.bDetectNeutrals);
		}
		if (hearing)
		{
			TestEqual(TEXT("Guard Hearing Range"), hearing->HearingRange, 5000.0f);
			TestEqual(TEXT("Guard Hearing MaxAge is Never"), hearing->GetMaxAge(),
				FAIStimulus::NeverHappenedAge);
			TestTrue(TEXT("Guard Hearing detects enemies"), hearing->DetectionByAffiliation.bDetectEnemies);
			TestTrue(TEXT("Guard Hearing detects friendlies"), hearing->DetectionByAffiliation.bDetectFriendlies);
			TestTrue(TEXT("Guard Hearing detects neutrals"), hearing->DetectionByAffiliation.bDetectNeutrals);
		}

		bool bAutoActivate = true;
		bool bAutoStart = true;
		TestTrue(TEXT("AIPerception AutoActivate property is readable"),
			ReadBoolProperty(perceptionComponents[0], TEXT("bAutoActivate"), bAutoActivate));
		TestFalse(TEXT("Guard AIPerception AutoActivate is disabled"), bAutoActivate);
		TestTrue(TEXT("StateTree AutoStart property is readable"),
			ReadBoolProperty(stateTreeComponents[0], TEXT("bStartLogicAutomatically"), bAutoStart));
		TestFalse(TEXT("Guard StateTree AutoStart is disabled"), bAutoStart);
		TestFalse(TEXT("Guard Perception is inactive before possession"), perceptionComponents[0]->IsActive());
		TestFalse(TEXT("Guard StateTree is stopped before possession"), stateTreeComponents[0]->IsRunning());
		TestTrue(TEXT("Guard StateTree is ST_Guard"), ReadConfiguredStateTree(stateTreeComponents[0])
			== LoadObject<UStateTree>(nullptr, TEXT("/Game/LostRunic/Blueprints/Guard/ST_Guard.ST_Guard")));
	}
	FString validationError;
	const bool bConfigurationValid = controller->ValidateControllerConfiguration(validationError, false);
	TestTrue(*FString::Printf(TEXT("Guard Blueprint configuration validates: %s"),
		*validationError), bConfigurationValid);
	DestroyContractWorld(world);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLRNPCControllerBlueprintContractTest,
	"LostRunic.AI.NPCControllerBlueprintContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLRNPCControllerBlueprintContractTest::RunTest(const FString& parameters)
{
	UBlueprint* controllerBlueprint = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/LostRunic/Blueprints/Character/BP_LRNPCController.BP_LRNPCController"));
	UClass* controllerClass = LoadClass<ALRNPCController>(nullptr,
		TEXT("/Game/LostRunic/Blueprints/Character/BP_LRNPCController.BP_LRNPCController_C"));
	UClass* pawnClass = LoadClass<ALRNPCCharacter>(nullptr,
		TEXT("/Game/LostRunic/Blueprints/Character/BP_NPC1.BP_NPC1_C"));
	if (!TestNotNull(TEXT("NPC Controller Blueprint loads"), controllerBlueprint)
		|| !TestNotNull(TEXT("NPC Controller generated class loads"), controllerClass)
		|| !TestNotNull(TEXT("NPC Pawn class loads"), pawnClass))
	{
		return false;
	}
	TestTrue(TEXT("NPC Controller parent is ALRNPCController"),
		controllerClass->GetSuperClass() == ALRNPCController::StaticClass());
	TestFalse(TEXT("NPC Controller SCS adds no AI components"),
		HasForbiddenAIComponentNode(controllerBlueprint));

	UWorld* world = CreateContractWorld();
	FActorSpawnParameters spawnParameters;
	spawnParameters.ObjectFlags = RF_Transient;
	ALRNPCController* controller = world
		? world->SpawnActor<ALRNPCController>(controllerClass, FTransform::Identity, spawnParameters) : nullptr;
	ALRNPCCharacter* pawn = world
		? world->SpawnActor<ALRNPCCharacter>(pawnClass, FTransform::Identity, spawnParameters) : nullptr;
	if (!TestNotNull(TEXT("NPC Controller instance spawns"), controller)
		|| !TestNotNull(TEXT("NPC Pawn instance spawns"), pawn))
	{
		DestroyContractWorld(world);
		return false;
	}

	TArray<UAIPerceptionComponent*> perceptionComponents;
	TArray<UStateTreeAIComponent*> stateTreeComponents;
	controller->GetComponents(perceptionComponents);
	controller->GetComponents(stateTreeComponents);
	TestEqual(TEXT("NPC Controller has one AIPerception"), perceptionComponents.Num(), 1);
	TestEqual(TEXT("NPC Controller has one StateTreeAI"), stateTreeComponents.Num(), 1);
	TestFalse(TEXT("NPC Pawn has no AIPerception"), pawn->FindComponentByClass<UAIPerceptionComponent>() != nullptr);
	TestFalse(TEXT("NPC Pawn has no StateTreeAI"), pawn->FindComponentByClass<UStateTreeAIComponent>() != nullptr);
	TestTrue(TEXT("NPC Pawn uses BP_LRNPCController"), pawn->AIControllerClass == controllerClass);

	if (perceptionComponents.Num() == 1 && stateTreeComponents.Num() == 1)
	{
		const UAISenseConfig_Hearing* hearing = perceptionComponents[0]->GetSenseConfig<UAISenseConfig_Hearing>();
		TestNotNull(TEXT("NPC Hearing is configured"), hearing);
		TestNull(TEXT("NPC Sight is absent"), perceptionComponents[0]->GetSenseConfig<UAISenseConfig_Sight>());
		TestTrue(TEXT("NPC dominant sense is Hearing"),
			perceptionComponents[0]->GetDominantSense() == UAISense_Hearing::StaticClass());
		if (hearing)
		{
			TestEqual(TEXT("NPC Hearing Range"), hearing->HearingRange, 5000.0f);
			TestEqual(TEXT("NPC Hearing MaxAge is Never"), hearing->GetMaxAge(),
				FAIStimulus::NeverHappenedAge);
			TestTrue(TEXT("NPC Hearing detects enemies"), hearing->DetectionByAffiliation.bDetectEnemies);
			TestTrue(TEXT("NPC Hearing detects friendlies"), hearing->DetectionByAffiliation.bDetectFriendlies);
			TestTrue(TEXT("NPC Hearing detects neutrals"), hearing->DetectionByAffiliation.bDetectNeutrals);
		}
		bool bAutoActivate = true;
		bool bAutoStart = true;
		ReadBoolProperty(perceptionComponents[0], TEXT("bAutoActivate"), bAutoActivate);
		ReadBoolProperty(stateTreeComponents[0], TEXT("bStartLogicAutomatically"), bAutoStart);
		TestFalse(TEXT("NPC AIPerception AutoActivate is disabled"), bAutoActivate);
		TestFalse(TEXT("NPC StateTree AutoStart is disabled"), bAutoStart);
		TestFalse(TEXT("NPC Perception is inactive before possession"), perceptionComponents[0]->IsActive());
		TestFalse(TEXT("NPC StateTree is stopped before possession"), stateTreeComponents[0]->IsRunning());
		TestTrue(TEXT("NPC StateTree is ST_NPC_Stand"), ReadConfiguredStateTree(stateTreeComponents[0])
			== LoadObject<UStateTree>(nullptr, TEXT("/Game/LostRunic/Blueprints/Guard/ST_NPC_Stand.ST_NPC_Stand")));
	}
	FString validationError;
	TestTrue(TEXT("NPC Blueprint configuration validates"),
		controller->ValidateControllerConfiguration(validationError, false));
	DestroyContractWorld(world);
	return true;
}

#endif
