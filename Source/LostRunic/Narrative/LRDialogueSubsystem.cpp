/**
 * @file LRDialogueSubsystem.cpp
 * @brief 读取 Dialogue/Reading DataTable，计算剧情条件与选项分支，记录一次性事件，并向 UI 发布当前台词、阅读内容及结束事件。
 *
 * 关联文件：LRDialogueSubsystem.h；所属领域：Narrative。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Narrative/LRDialogueSubsystem.h"

#include "Core/LRGameplayTags.h"
#include "Core/LRLog.h"
#include "Data/LRContentRows.h"
#include "Data/LRGameContentSet.h"
#include "Data/LRLevelEventDefinition.h"
#include "Engine/DataTable.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "Narrative/LRNarrativeRules.h"

/**
 * @brief 初始化子系统拥有的长期状态与事件绑定。
 * @param collection 调用方提供的 `collection`，只在本次操作范围内使用。
 */
void ULRDialogueSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
	Super::Initialize(collection);
	collection.InitializeDependency<ULRGameInstanceSubsystem>();
	const ULRGameInstanceSubsystem* dataSubsystem = GetGameInstance()->GetSubsystem<ULRGameInstanceSubsystem>();
	InitializeContent(dataSubsystem ? dataSubsystem->GetContentSet() : nullptr);
}

/**
 * @brief 释放子系统事件绑定和运行时缓存。
 */
void ULRDialogueSubsystem::Deinitialize()
{
	ResetSession();
	ContentSet = nullptr;
	ContextTags.Reset();
	CompletedEventIds.Reset();
	Super::Deinitialize();
}

/**
 * @brief 从 LRGameContentSet 建立 DataTable 与定义资产索引，缺失内容返回可诊断错误。
 * @param contentSet 数据或调优来源 `contentSet`；调用期间只读，并按稳定 ID 解析内容。
 */
void ULRDialogueSubsystem::InitializeContent(ULRGameContentSet* contentSet)
{
	ContentSet = contentSet;
	ResetSession();
}

/**
 * @brief 使用稳定 Dialogue 行 ID 启动对话会话并发布首个页面。
 * @param rowId DataTable 稳定行 ID，不使用行号。
 * @param completionEventId 稳定标识 `completionEventId`；用于内容查询和存档，不依赖显示名或数组序号。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FLRNarrativeResult ULRDialogueSubsystem::StartDialogue(const FName rowId, const FName completionEventId)
{
	ResetSession();
	CompletionEventId = completionEventId;
	FLRNarrativeResult result = ShowDialogueRow(rowId, ELRNarrativeAction::Started);
	if (!result.bSuccess)
	{
		ResetSession();
	}
	return result;
}

/**
 * @brief 使用稳定 Reading 行 ID 启动阅读会话，并复用叙事推进规则。
 * @param readingId 稳定标识 `readingId`；用于内容查询和存档，不依赖显示名或数组序号。
 * @param completionEventId 稳定标识 `completionEventId`；用于内容查询和存档，不依赖显示名或数组序号。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FLRNarrativeResult ULRDialogueSubsystem::StartReading(const FName readingId, const FName completionEventId)
{
	ResetSession();
	CompletionEventId = completionEventId;
	FLRNarrativeResult result = ShowReadingRow(readingId);
	if (!result.bSuccess)
	{
		ResetSession();
	}
	return result;
}

/**
 * @brief 推进当前对话或阅读会话；根据条件选择下一行并处理一次性事件。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FLRNarrativeResult ULRDialogueSubsystem::Advance()
{
	if (!HasActiveSession())
	{
		return Reject(NAME_None, LRGameplayTags::NarrativeRejectNoSession);
	}
	if (CurrentPage.SessionType == ELRNarrativeSessionType::Reading)
	{
		return FinishSession();
	}
	if (!CurrentPage.Choices.IsEmpty())
	{
		FLRNarrativeResult result;
		result.bSuccess = true;
		result.Action = ELRNarrativeAction::AwaitChoice;
		result.ContentId = CurrentPage.ContentId;
		return result;
	}

	const FLRDialogueRow* row = ContentSet && ContentSet->DialogueTable
		? ContentSet->DialogueTable->FindRow<FLRDialogueRow>(CurrentPage.ContentId, TEXT("Advance dialogue")) : nullptr;
	if (!row)
	{
		return Reject(CurrentPage.ContentId, LRGameplayTags::NarrativeRejectMissingContent);
	}
	if (!row->Options.IsEmpty())
	{
		return Reject(CurrentPage.ContentId, LRGameplayTags::NarrativeRejectConditions);
	}
	return row->NextRowId.IsNone() ? FinishSession()
		: ShowDialogueRow(row->NextRowId, ELRNarrativeAction::Advanced);
}

/**
 * @brief 执行 Select Choice 的纯规则或事务判定，失败时提供结构化原因。
 * @param choiceId 稳定标识 `choiceId`；用于内容查询和存档，不依赖显示名或数组序号。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FLRNarrativeResult ULRDialogueSubsystem::SelectChoice(const FName choiceId)
{
	if (CurrentPage.SessionType != ELRNarrativeSessionType::Dialogue)
	{
		return Reject(CurrentPage.ContentId, LRGameplayTags::NarrativeRejectNoSession);
	}
	const FLRNarrativeChoice* choice = CurrentPage.Choices.FindByPredicate([choiceId](const FLRNarrativeChoice& candidate)
	{
		return candidate.ChoiceId == choiceId;
	});
	if (!choice)
	{
		return Reject(CurrentPage.ContentId, LRGameplayTags::NarrativeRejectInvalidChoice);
	}
	return choice->NextContentId.IsNone() ? FinishSession()
		: ShowDialogueRow(choice->NextContentId, ELRNarrativeAction::Advanced);
}

/**
 * @brief 结束或取消 End Session 流程，并清理本次操作拥有的临时状态。
 */
void ULRDialogueSubsystem::EndSession()
{
	if (!HasActiveSession())
	{
		return;
	}
	const ELRNarrativeSessionType sessionType = CurrentPage.SessionType;
	const FName finalContentId = CurrentPage.ContentId;
	ResetSession();
	OnSessionEnded.Broadcast(sessionType, finalContentId);
}

/**
 * @brief 解析指定对话行、条件和选项并发布给 UI，不在 C++ 中硬编码台词。
 * @param rowId DataTable 稳定行 ID，不使用行号。
 * @param action 输入动作或数值 `action`；不包含写死的具体键位。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FLRNarrativeResult ULRDialogueSubsystem::ShowDialogueRow(const FName rowId, const ELRNarrativeAction action)
{
	const FLRDialogueRow* row = ContentSet && ContentSet->DialogueTable
		? ContentSet->DialogueTable->FindRow<FLRDialogueRow>(rowId, TEXT("Show dialogue")) : nullptr;
	if (!row)
	{
		return Reject(rowId, LRGameplayTags::NarrativeRejectMissingContent);
	}
	if (!LRNarrativeRules::AreConditionsMet(row->RequiredTags, row->BlockedTags, ContextTags))
	{
		return Reject(rowId, LRGameplayTags::NarrativeRejectConditions);
	}
	if (row->DialogueId != rowId)
	{
		UE_LOG(LogLostRunicNarrative, Warning, TEXT("Dialogue row name=%s has mismatched ID=%s"),
			*rowId.ToString(), *row->DialogueId.ToString());
		return Reject(rowId, LRGameplayTags::NarrativeRejectMissingContent);
	}

	CurrentPage = FLRNarrativePage();
	CurrentPage.SessionType = ELRNarrativeSessionType::Dialogue;
	CurrentPage.ContentId = row->DialogueId;
	CurrentPage.SpeakerId = row->SpeakerId;
	CurrentPage.Text = row->Text;
	CurrentPage.Portrait = row->Portrait;
	LRNarrativeRules::BuildAvailableChoices(row->Options, ContextTags, CurrentPage.Choices);
	OnPageChanged.Broadcast(CurrentPage);

	FLRNarrativeResult result;
	result.bSuccess = true;
	result.Action = action;
	result.ContentId = CurrentPage.ContentId;
	return result;
}

/**
 * @brief 解析指定阅读行并发布给 UI，同时记录稳定笔记 ID。
 * @param readingId 稳定标识 `readingId`；用于内容查询和存档，不依赖显示名或数组序号。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FLRNarrativeResult ULRDialogueSubsystem::ShowReadingRow(const FName readingId)
{
	const FLRReadingRow* row = ContentSet && ContentSet->ReadingTable
		? ContentSet->ReadingTable->FindRow<FLRReadingRow>(readingId, TEXT("Show reading")) : nullptr;
	if (!row)
	{
		return Reject(readingId, LRGameplayTags::NarrativeRejectMissingContent);
	}
	if (row->ReadingId != readingId)
	{
		UE_LOG(LogLostRunicNarrative, Warning, TEXT("Reading row name=%s has mismatched ID=%s"),
			*readingId.ToString(), *row->ReadingId.ToString());
		return Reject(readingId, LRGameplayTags::NarrativeRejectMissingContent);
	}

	CurrentPage = FLRNarrativePage();
	CurrentPage.SessionType = ELRNarrativeSessionType::Reading;
	CurrentPage.ContentId = row->ReadingId;
	CurrentPage.Title = row->Title;
	CurrentPage.Text = row->Body;
	OnPageChanged.Broadcast(CurrentPage);

	FLRNarrativeResult result;
	result.bSuccess = true;
	result.Action = ELRNarrativeAction::Started;
	result.ContentId = CurrentPage.ContentId;
	return result;
}

/**
 * @brief 结束当前对话或阅读会话，提交最终事件并广播会话结束。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FLRNarrativeResult ULRDialogueSubsystem::FinishSession()
{
	const FName eventId = CompletionEventId;
	const ELRNarrativeSessionType sessionType = CurrentPage.SessionType;
	const FName finalContentId = CurrentPage.ContentId;
	ResetSession();
	OnSessionEnded.Broadcast(sessionType, finalContentId);

	if (!eventId.IsNone())
	{
		FLRNarrativeResult eventResult = TryCompleteEvent(eventId);
		if (!eventResult.bSuccess)
		{
			return eventResult;
		}
	}
	FLRNarrativeResult result;
	result.bSuccess = true;
	result.Action = ELRNarrativeAction::Completed;
	result.ContentId = finalContentId;
	return result;
}

/**
 * @brief 创建带原因 Gameplay Tag 的结构化失败结果，并保留事务不变量。
 * @param contentId 稳定标识 `contentId`；用于内容查询和存档，不依赖显示名或数组序号。
 * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FLRNarrativeResult ULRDialogueSubsystem::Reject(const FName contentId, const FGameplayTag reason)
{
	FLRNarrativeResult result;
	result.ContentId = contentId;
	result.FailureReason = reason;
	OnRequestRejected.Broadcast(contentId, reason);
	UE_LOG(LogLostRunicNarrative, Verbose, TEXT("NarrativeContent=%s rejected reason=%s"),
		*contentId.ToString(), *reason.ToString());
	return result;
}

/**
 * @brief 清空当前对话/阅读行、选项和表现状态，但保留已完成剧情事件。
 */
void ULRDialogueSubsystem::ResetSession()
{
	CurrentPage = FLRNarrativePage();
	CompletionEventId = NAME_None;
}
