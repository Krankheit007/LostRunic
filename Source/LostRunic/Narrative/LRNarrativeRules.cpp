/**
 * @file LRNarrativeRules.cpp
 * @brief 实现 DataTable 驱动的对话、阅读、条件分支和一次性剧情事件；稳定 FName ID 进入存档，显示全文与推进下一句的二段确认由控制层维护。
 *
 * 关联文件：LRNarrativeRules.h；所属领域：Narrative。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Narrative/LRNarrativeRules.h"

#include "Data/LRContentRows.h"
#include "Narrative/LRNarrativeTypes.h"

/**
 * @brief 根据已完成事件、所需标签和阻止标签判断叙事内容是否可用。
 * @param requiredTags Gameplay Tag 或标签集合，用于分类、条件、拒绝原因和可诊断事件。
 * @param blockedTags Gameplay Tag 或标签集合，用于分类、条件、拒绝原因和可诊断事件。
 * @param contextTags Gameplay Tag 或标签集合，用于分类、条件、拒绝原因和可诊断事件。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool LRNarrativeRules::AreConditionsMet(const FGameplayTagContainer& requiredTags,
	const FGameplayTagContainer& blockedTags, const FGameplayTagContainer& contextTags)
{
	return contextTags.HasAll(requiredTags) && !contextTags.HasAny(blockedTags);
}

/**
 * @brief 筛选当前对话行的可用选项，只返回满足剧情条件的稳定选择 ID。
 * @param options 本次领域操作的结构化数据 `options`；字段语义由对应 USTRUCT 定义。
 * @param contextTags Gameplay Tag 或标签集合，用于分类、条件、拒绝原因和可诊断事件。
 * @param outChoices 本次领域操作的结构化数据 `outChoices`；字段语义由对应 USTRUCT 定义。
 */
void LRNarrativeRules::BuildAvailableChoices(const TArray<FLRDialogueOption>& options,
	const FGameplayTagContainer& contextTags, TArray<FLRNarrativeChoice>& outChoices)
{
	outChoices.Reset();
	for (const FLRDialogueOption& option : options)
	{
		if (!AreConditionsMet(option.RequiredTags, option.BlockedTags, contextTags))
		{
			continue;
		}

		FLRNarrativeChoice& choice = outChoices.AddDefaulted_GetRef();
		choice.ChoiceId = option.OptionId;
		choice.Text = option.Text;
		choice.NextContentId = option.NextRowId;
	}
}
