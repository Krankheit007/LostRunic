#include "Core/LRLog.h"

#include "Data/LRGameTuningSet.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "HAL/IConsoleManager.h"

namespace
{
	ULRGameInstanceSubsystem* GetLRSubsystem(UWorld* world)
	{
		UGameInstance* gameInstance = world ? world->GetGameInstance() : nullptr;
		return gameInstance ? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	}

	void DumpState(UWorld* world)
	{
		UE_LOG(LogLostRunicState, Display, TEXT("World=%s State provider is not active until the LR player framework is spawned."), *GetNameSafe(world));
	}

	void DumpAlert(UWorld* world)
	{
		UE_LOG(LogLostRunicAI, Display, TEXT("World=%s Guard debug drawing is enabled by active LR guard components."), *GetNameSafe(world));
	}

	void DumpInteraction(UWorld* world)
	{
		UE_LOG(LogLostRunicInteraction, Display, TEXT("World=%s Interaction diagnostics require an active LR interaction component."), *GetNameSafe(world));
	}

	void DumpSave(UWorld* world)
	{
		UE_LOG(LogLostRunicSave, Display, TEXT("World=%s Save diagnostics require the LR save subsystem."), *GetNameSafe(world));
	}

	void DumpTuning(UWorld* world)
	{
		const ULRGameInstanceSubsystem* subsystem = GetLRSubsystem(world);
		const ULRGameTuningSet* tuningSet = subsystem ? subsystem->GetTuningSet() : nullptr;
		if (!tuningSet)
		{
			UE_LOG(LogLostRunicTuning, Warning, TEXT("World=%s has no loaded LR tuning set."), *GetNameSafe(world));
			return;
		}

		tuningSet->LogSources();
	}

	FAutoConsoleCommandWithWorld StateCommand(
		TEXT("LR.Debug.State"), TEXT("Print the current LostRunic state and latest transition reason."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&DumpState));
	FAutoConsoleCommandWithWorld AlertCommand(
		TEXT("LR.Debug.Alert"), TEXT("Print and draw LostRunic guard alert diagnostics."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&DumpAlert));
	FAutoConsoleCommandWithWorld InteractionCommand(
		TEXT("LR.Debug.Interaction"), TEXT("Print LostRunic interaction candidates and rejection reasons."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&DumpInteraction));
	FAutoConsoleCommandWithWorld SaveCommand(
		TEXT("LR.Debug.Save"), TEXT("Print LostRunic save slot and transaction diagnostics."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&DumpSave));
	FAutoConsoleCommandWithWorld TuningCommand(
		TEXT("LR.Debug.Tuning"), TEXT("Print LostRunic tuning sources and effective values."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&DumpTuning));
}
