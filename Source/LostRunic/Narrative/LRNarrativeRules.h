/**
 * @file LRNarrativeRules.h
 * @brief 实现 DataTable 驱动的对话、阅读、条件分支和一次性剧情事件；稳定 FName ID 进入存档，显示全文与推进下一句的二段确认由控制层维护。
 *
 * 关联文件：LRNarrativeRules.cpp；所属领域：Narrative。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "CoreMinimal.h"

struct FLRDialogueOption;
struct FLRNarrativeChoice;
struct FGameplayTagContainer;

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
namespace LRNarrativeRules
{
	LOSTRUNIC_API bool AreConditionsMet(const FGameplayTagContainer& requiredTags,
		const FGameplayTagContainer& blockedTags, const FGameplayTagContainer& contextTags);

	LOSTRUNIC_API void BuildAvailableChoices(const TArray<FLRDialogueOption>& options,
		const FGameplayTagContainer& contextTags, TArray<FLRNarrativeChoice>& outChoices);
}
