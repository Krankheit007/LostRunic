/**
 * @file LRDialogueSubsystemEvents.cpp
 * @brief 实现 DataTable 驱动的对话、阅读、条件分支和一次性剧情事件；稳定 FName ID 进入存档，显示全文与推进下一句的二段确认由控制层维护。
 *
 * 关联文件：Narrative 目录内调用该公共契约的实现文件；所属领域：Narrative。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Narrative/LRDialogueSubsystem.h"

#include "Core/LRGameplayTags.h"
#include "Core/LRLog.h"
#include "Data/LRGameContentSet.h"
#include "Data/LRLevelEventDefinition.h"
#include "Narrative/LRNarrativeRules.h"

/**
 * @brief 检查关卡事件条件和一次性标记，成功后提交剧情进度及其显式存档策略。
 * @param eventId 剧情事件的稳定 FName ID，用于一次性判定和存档。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FLRNarrativeResult ULRDialogueSubsystem::TryCompleteEvent(const FName eventId)
{
	ULRLevelEventDefinition* definition = FindEventDefinition(eventId);
	if (!definition)
	{
		return Reject(eventId, LRGameplayTags::NarrativeRejectMissingContent);
	}
	if (!LRNarrativeRules::AreConditionsMet(definition->RequiredTags, definition->BlockedTags, ContextTags))
	{
		return Reject(eventId, LRGameplayTags::NarrativeRejectConditions);
	}
	if (definition->bOneShot && CompletedEventIds.Contains(eventId))
	{
		return Reject(eventId, LRGameplayTags::NarrativeRejectAlreadyCompleted);
	}

	CompletedEventIds.Add(eventId);
	OnEventCommitted.Broadcast(eventId, definition->SavePolicy);
	UE_LOG(LogLostRunicNarrative, Log, TEXT("NarrativeEvent=%s completed savePolicy=%d"),
		*eventId.ToString(), static_cast<int32>(definition->SavePolicy));
	FLRNarrativeResult result;
	result.bSuccess = true;
	result.Action = ELRNarrativeAction::Completed;
	result.ContentId = eventId;
	return result;
}

/**
 * @brief 更新 Context Tags，并在需要时同步组件状态或广播变化事件。
 * @param contextTags Gameplay Tag 或标签集合，用于分类、条件、拒绝原因和可诊断事件。
 */
void ULRDialogueSubsystem::SetContextTags(const FGameplayTagContainer& contextTags)
{
	ContextTags = contextTags;
}

/**
 * @brief 把 Restore Completed Events 数据应用到运行时对象，并显式处理缺失依赖。
 * @param eventIds 调用方提供的 `eventIds`，只在本次操作范围内使用。
 */
void ULRDialogueSubsystem::RestoreCompletedEvents(const TSet<FName>& eventIds)
{
	CompletedEventIds = eventIds;
}

/**
 * @brief 按稳定 ID 或运行时条件查找 Event Definition，未找到时返回明确失败值。
 * @param eventId 剧情事件的稳定 FName ID，用于一次性判定和存档。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
ULRLevelEventDefinition* ULRDialogueSubsystem::FindEventDefinition(const FName eventId) const
{
	if (!ContentSet || eventId.IsNone())
	{
		return nullptr;
	}
	const TObjectPtr<ULRLevelEventDefinition>* found = ContentSet->LevelEvents.FindByPredicate(
		[eventId](const ULRLevelEventDefinition* definition)
		{
			return definition && definition->EventId == eventId;
		});
	return found ? found->Get() : nullptr;
}
