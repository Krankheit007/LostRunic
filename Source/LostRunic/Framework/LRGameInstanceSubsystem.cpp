/**
 * @file LRGameInstanceSubsystem.cpp
 * @brief 连接 LostRunic 的 Gameplay Framework：GameMode 管理单机世界规则，PlayerController 解释 Enhanced Input 与 UI 模式，Character 只组合能力组件，GameInstanceSubsystem 提供跨地图内容与调优配置。
 *
 * 关联文件：LRGameInstanceSubsystem.h；所属领域：Framework。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Framework/LRGameInstanceSubsystem.h"

#include "Core/LRLog.h"
#include "Data/LRGameContentSet.h"
#include "Data/LRGameTuningSet.h"
#include "Data/LRProjectSettings.h"
#include "Narrative/LRDialogueScriptRegistry.h"

/**
 * @brief 初始化子系统拥有的长期状态与事件绑定。
 * @param collection 调用方提供的 `collection`，只在本次操作范围内使用。
 */
void ULRGameInstanceSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
	Super::Initialize(collection);

	const ULRProjectSettings* settings = GetDefault<ULRProjectSettings>();
	TuningSet = settings->TuningSet.LoadSynchronous();
	ContentSet = settings->ContentSet.LoadSynchronous();

	FString tuningError;
	FString contentError;
	const bool bTuningValid = TuningSet && TuningSet->Validate(tuningError);
	bool bContentValid = ContentSet && ContentSet->Validate(contentError);
	if (bContentValid)
	{
		FString registryError;
		if (!ContentSet->DialogueScriptRegistry->Validate(registryError))
		{
			contentError = FString::Printf(TEXT("DialogueScriptRegistry is invalid: %s"), *registryError);
			bContentValid = false;
		}
	}
	bConfigurationValid = bTuningValid && bContentValid;

	if (!bConfigurationValid)
	{
		UE_LOG(LogLostRunicTuning, Error, TEXT("GameInstance=%s invalid data roots. Tuning='%s' Content='%s'"),
			*GetNameSafe(GetGameInstance()), *tuningError, *contentError);
		return;
	}

	TuningSet->LogSources();
}

/**
 * @brief 释放子系统事件绑定和运行时缓存。
 */
void ULRGameInstanceSubsystem::Deinitialize()
{
	bConfigurationValid = false;
	ContentSet = nullptr;
	TuningSet = nullptr;
	Super::Deinitialize();
}
