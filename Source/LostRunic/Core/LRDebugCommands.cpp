/**
 * @file LRDebugCommands.cpp
 * @brief 声明 LostRunic 各玩法领域共享的稳定 ID、Gameplay Tags、日志分类、数据校验与调试命令，供状态、交互、AI、叙事和存档系统统一使用。
 *
 * 关联文件：Core 目录内调用该公共契约的实现文件；所属领域：Core。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Core/LRLog.h"

#include "AI/LRGuardAIController.h"
#include "Data/LRGameTuningSet.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "Framework/LRCharacter.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Interaction/LRInteractionComponent.h"
#include "State/LRStateComponent.h"
#include "EngineUtils.h"

namespace
{
	/**
	 * @brief 查询 LRSubsystem；不修改领域状态。
	 * @param world 要解析地图 ID、应用恢复状态或执行查询的 Unreal World。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	ULRGameInstanceSubsystem* GetLRSubsystem(UWorld* world)
	{
		UGameInstance* gameInstance = world ? world->GetGameInstance() : nullptr;
		return gameInstance ? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	}

	/**
	 * @brief 执行 LR.Debug.State，输出当前心理状态、阻塞标签和最近切换原因。
	 * @param world 要解析地图 ID、应用恢复状态或执行查询的 Unreal World。
	 */
	void DumpState(UWorld* world)
	{
		const ALRCharacter* character = Cast<ALRCharacter>(UGameplayStatics::GetPlayerCharacter(world, 0));
		const ULRStateComponent* state = character ? character->GetStateComponent() : nullptr;
		if (!state)
		{
			UE_LOG(LogLostRunicState, Display, TEXT("World=%s has no active LR player state."), *GetNameSafe(world));
			return;
		}
		state->LogDiagnostics();
	}

	/**
	 * @brief 执行 LR.Debug.Alert，输出并绘制所有 LR 守卫视野、听觉和警戒诊断。
	 * @param world 要解析地图 ID、应用恢复状态或执行查询的 Unreal World。
	 */
	void DumpAlert(UWorld* world)
	{
		int32 guardCount = 0;
		for (TActorIterator<ALRGuardAIController> controllerIt(world); controllerIt; ++controllerIt)
		{
			controllerIt->LogAndDrawDiagnostics();
			++guardCount;
		}
		UE_LOG(LogLostRunicAI, Display, TEXT("World=%s drew diagnostics for %d LR guards."),
			*GetNameSafe(world), guardCount);
	}

	/**
	 * @brief 执行 LR.Debug.Interaction，输出当前交互候选、唯一目标和拒绝原因。
	 * @param world 要解析地图 ID、应用恢复状态或执行查询的 Unreal World。
	 */
	void DumpInteraction(UWorld* world)
	{
		const ALRCharacter* character = Cast<ALRCharacter>(UGameplayStatics::GetPlayerCharacter(world, 0));
		const ULRInteractionComponent* interaction = character
			? character->FindComponentByClass<ULRInteractionComponent>() : nullptr;
		if (interaction)
		{
			interaction->LogDiagnostics();
			return;
		}
		UE_LOG(LogLostRunicInteraction, Display, TEXT("World=%s has no active LR interaction component."), *GetNameSafe(world));
	}

	/**
	 * @brief 执行 LR.Debug.Save，输出当前世界对应的存档诊断入口。
	 * @param world 要解析地图 ID、应用恢复状态或执行查询的 Unreal World。
	 */
	void DumpSave(UWorld* world)
	{
		UE_LOG(LogLostRunicSave, Display, TEXT("World=%s Save diagnostics require the LR save subsystem."), *GetNameSafe(world));
	}

	/**
	 * @brief 执行 LR.Debug.Tuning，输出聚合调优资产和各领域实际来源。
	 * @param world 要解析地图 ID、应用恢复状态或执行查询的 Unreal World。
	 */
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
