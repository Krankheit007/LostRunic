/**
 * @file LRDialogueSubsystem.cpp
 * @brief 驱动 SUDS Dialogue 与 Reading DataTable 会话，记录剧情状态，并向 UI 发布当前台词、阅读内容及结束事件。
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
#include "Narrative/LRDialogueEventBridge.h"
#include "Narrative/LRDialogueScriptRegistry.h"
#include "Narrative/LRDialogueStateParticipant.h"
#include "Narrative/LRDialogueSpeakerRegistry.h"
#include "Narrative/LRStoryStateSubsystem.h"
#include "SUDSCommon.h"
#include "SUDSDialogue.h"
#include "SUDSLibrary.h"
#include "SUDSScript.h"
#include "SUDSScriptEdge.h"

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
	EndSUDSDialogue(ELRDialogueEndReason::LevelTravel);
	ResetSession();
	ContentSet = nullptr;
	DialogueScriptRegistry = nullptr;
	ContextTags.Reset();
	CompletedEventIds.Reset();
	MemoryEventIds.Reset();
	Super::Deinitialize();
}

FLRNarrativeResult ULRDialogueSubsystem::StartSUDSDialogue(const FLRDialogueStartRequest& request)
{
	if (HasActiveSession() || ActiveSUDSDialogue)
	{
		return Reject(request.ScriptId, LRGameplayTags::NarrativeRejectConditions);
	}
	if (request.ScriptId.IsNone() || !request.Script || !DialogueScriptRegistry)
	{
		return Reject(request.ScriptId, LRGameplayTags::NarrativeRejectMissingContent);
	}
	TObjectPtr<USUDSScript> RegisteredScript = nullptr;
	FString RegistryError;
	const bool bResolved = DialogueScriptRegistry->Resolve(request.ScriptId, RegisteredScript, RegistryError);
	if (!bResolved)
	{
		UE_LOG(LogLostRunicNarrative, Warning, TEXT("Dialogue request rejected ScriptId=%s because ScriptId and Script do not match the global Registry: %s"),
			*request.ScriptId.ToString(), *RegistryError);
		return Reject(request.ScriptId, LRGameplayTags::NarrativeRejectMissingContent);
	}
	if (RegisteredScript != request.Script)
	{
		RegistryError = TEXT("The supplied Script pointer does not match the ScriptId mapping.");
		UE_LOG(LogLostRunicNarrative, Warning, TEXT("Dialogue request rejected ScriptId=%s because ScriptId and Script do not match the global Registry: %s"),
			*request.ScriptId.ToString(), *RegistryError);
		return Reject(request.ScriptId, LRGameplayTags::NarrativeRejectMissingContent);
	}
	UGameInstance* gameInstance = GetGameInstance();
	ULRStoryStateSubsystem* storyState = gameInstance
		? gameInstance->GetSubsystem<ULRStoryStateSubsystem>() : nullptr;
	if (request.CompletionStoryTag.IsValid() && storyState && storyState->HasStoryFlag(request.CompletionStoryTag))
	{
		return Reject(request.ScriptId, LRGameplayTags::NarrativeRejectAlreadyCompleted);
	}

	ActiveEndReason = ELRDialogueEndReason::None;
	ActiveScriptId = request.ScriptId;
	ActiveSUDSScript = request.Script;
	ActiveCompletionStoryTag = request.CompletionStoryTag;
	ActiveDialogueOwner = request.Owner.IsValid() ? request.Owner : this;
	DialogueStateParticipant = NewObject<ULRDialogueStateParticipant>(this);
	DialogueStateParticipant->Initialize(storyState);
	DialogueEventBridge = NewObject<ULRDialogueEventBridge>(this);
	DialogueEventBridge->Initialize(storyState);
	ActiveSUDSDialogue = USUDSLibrary::CreateDialogue(ActiveDialogueOwner.Get(), request.Script, false, request.StartLabel);
	if (!ActiveSUDSDialogue)
	{
		EndSUDSDialogue(ELRDialogueEndReason::Error);
		return Reject(request.ScriptId, LRGameplayTags::NarrativeRejectMissingContent);
	}
	ActiveSUDSDialogue->AddParticipant(DialogueStateParticipant);
	ActiveSUDSDialogue->OnSpeakerLine.AddDynamic(this, &ULRDialogueSubsystem::HandleSUDSSpeakerLine);
	ActiveSUDSDialogue->OnChoice.AddDynamic(this, &ULRDialogueSubsystem::HandleSUDSChoice);
	ActiveSUDSDialogue->OnFinished.AddDynamic(this, &ULRDialogueSubsystem::HandleSUDSFinished);
	ActiveSUDSDialogue->OnEvent.AddDynamic(DialogueEventBridge, &ULRDialogueEventBridge::HandleDialogueEvent);
	ActiveSUDSDialogue->Start(request.StartLabel);

	FLRNarrativeResult result;
	result.bSuccess = ActiveSUDSDialogue != nullptr;
	result.Action = ELRNarrativeAction::Started;
	result.ContentId = request.ScriptId;
	return result;
}

FLRNarrativeResult ULRDialogueSubsystem::AdvanceSUDSDialogue()
{
	if (!ActiveSUDSDialogue || !HasActiveSession())
	{
		return Reject(ActiveScriptId, LRGameplayTags::NarrativeRejectNoSession);
	}
	if (!ActiveSUDSDialogue->IsSimpleContinue())
	{
		return Reject(ActiveScriptId, LRGameplayTags::NarrativeRejectInvalidChoice);
	}
	const bool bContinued = ActiveSUDSDialogue->Continue();
	FLRNarrativeResult result;
	result.bSuccess = bContinued || !ActiveSUDSDialogue;
	result.Action = bContinued ? ELRNarrativeAction::Advanced : ELRNarrativeAction::Completed;
	result.ContentId = ActiveScriptId;
	return result;
}

FLRNarrativeResult ULRDialogueSubsystem::SelectSUDSChoice(const int32 choiceIndex)
{
	if (!ActiveSUDSDialogue || !HasActiveSession())
	{
		return Reject(ActiveScriptId, LRGameplayTags::NarrativeRejectNoSession);
	}
	if (ActiveSUDSDialogue->IsSimpleContinue() || choiceIndex < 0
		|| choiceIndex >= ActiveSUDSDialogue->GetNumberOfChoices())
	{
		return Reject(ActiveScriptId, LRGameplayTags::NarrativeRejectInvalidChoice);
	}
	const bool bContinued = ActiveSUDSDialogue->Choose(choiceIndex);
	FLRNarrativeResult result;
	result.bSuccess = bContinued || !ActiveSUDSDialogue;
	result.Action = bContinued ? ELRNarrativeAction::Advanced : ELRNarrativeAction::Completed;
	result.ContentId = ActiveScriptId;
	return result;
}

void ULRDialogueSubsystem::EndSUDSDialogue(const ELRDialogueEndReason reason, UObject* owner)
{
	if (!ActiveSUDSDialogue)
	{
		return;
	}
	if (owner && ActiveDialogueOwner.IsValid() && owner != ActiveDialogueOwner.Get())
	{
		return;
	}
	ActiveEndReason = reason;
	const ELRNarrativeSessionType sessionType = CurrentPage.SessionType;
	const FName finalContentId = CurrentPage.ContentId;
	ActiveSUDSDialogue->End(true);
	ActiveSUDSDialogue = nullptr;
	ActiveSUDSScript = nullptr;
	DialogueEventBridge = nullptr;
	DialogueStateParticipant = nullptr;
	ActiveDialogueOwner.Reset();
	ActiveScriptId = NAME_None;
	ActiveCompletionStoryTag = FGameplayTag();
	ResetSession();
	if (sessionType != ELRNarrativeSessionType::None)
	{
		OnSessionEnded.Broadcast(sessionType, finalContentId);
	}
}

void ULRDialogueSubsystem::HandleSUDSSpeakerLine(USUDSDialogue* dialogue)
{
	if (!dialogue || dialogue != ActiveSUDSDialogue)
	{
		return;
	}
	CurrentPage = FLRNarrativePage();
	CurrentPage.SessionType = ELRNarrativeSessionType::Dialogue;
	CurrentPage.ContentId = ActiveScriptId;
	CurrentPage.ScriptId = ActiveScriptId;
	CurrentPage.Text = dialogue->GetText();
	CurrentPage.LineText = CurrentPage.Text;
	CurrentPage.TextId = SUDS_GET_TEXT_KEY(CurrentPage.Text);
	CurrentPage.SpeakerId = FName(*dialogue->GetSpeakerID());
	if (const FLRDialogueSpeakerDefinition* Speaker = DialogueSpeakerRegistry
		? DialogueSpeakerRegistry->Find(CurrentPage.SpeakerId) : nullptr)
	{
		CurrentPage.SpeakerName = Speaker->DisplayName;
		CurrentPage.Portrait = Speaker->Portrait;
		CurrentPage.bShowPortrait = IsValid(CurrentPage.Portrait);
	}
	else
	{
		CurrentPage.SpeakerName = dialogue->GetSpeakerDisplayName();
	}
	CurrentPage.bShowSpeakerName = !CurrentPage.SpeakerName.IsEmpty();
	CurrentPage.bSimpleContinue = dialogue->IsSimpleContinue();
	if (!CurrentPage.bSimpleContinue)
	{
		const TArray<FSUDSScriptEdge>& choices = dialogue->GetChoices();
		for (int32 index = 0; index < choices.Num(); ++index)
		{
			const FSUDSScriptEdge& edge = choices[index];
			FLRNarrativeChoice choice;
			choice.ChoiceIndex = index;
			choice.ChoiceId = FName(*edge.GetTextID());
			choice.Text = edge.GetText();
			CurrentPage.Choices.Add(MoveTemp(choice));
		}
	}
	OnPageChanged.Broadcast(CurrentPage);
}

void ULRDialogueSubsystem::HandleSUDSChoice(USUDSDialogue* dialogue, const int choiceIndex)
{
	if (dialogue == ActiveSUDSDialogue)
	{
		UE_LOG(LogLostRunicNarrative, Verbose, TEXT("Dialogue ScriptId=%s choiceIndex=%d selected."),
			*ActiveScriptId.ToString(), choiceIndex);
	}
}

void ULRDialogueSubsystem::HandleSUDSFinished(USUDSDialogue* dialogue)
{
	if (dialogue != ActiveSUDSDialogue)
	{
		return;
	}
	FinishSUDSSession(ActiveEndReason == ELRDialogueEndReason::None
		? ELRDialogueEndReason::CompletedNaturally : ActiveEndReason);
}

void ULRDialogueSubsystem::FinishSUDSSession(const ELRDialogueEndReason reason)
{
	if (!ActiveSUDSDialogue)
	{
		return;
	}
	ActiveEndReason = reason;
	const FName finalContentId = CurrentPage.ContentId;
	const FGameplayTag completionTag = ActiveCompletionStoryTag;
	UGameInstance* gameInstance = GetGameInstance();
	ULRStoryStateSubsystem* storyState = gameInstance
		? gameInstance->GetSubsystem<ULRStoryStateSubsystem>() : nullptr;
	if (reason == ELRDialogueEndReason::CompletedNaturally && completionTag.IsValid() && storyState)
	{
		storyState->AddStoryFlag(completionTag);
	}
	ActiveSUDSDialogue = nullptr;
	ActiveSUDSScript = nullptr;
	DialogueEventBridge = nullptr;
	DialogueStateParticipant = nullptr;
	ActiveDialogueOwner.Reset();
	ActiveScriptId = NAME_None;
	ActiveCompletionStoryTag = FGameplayTag();
	ResetSession();
	OnSessionEnded.Broadcast(ELRNarrativeSessionType::Dialogue, finalContentId);
}

/**
 * @brief 从 LRGameContentSet 建立 DataTable 与定义资产索引，缺失内容返回可诊断错误。
 * @param contentSet 数据或调优来源 `contentSet`；调用期间只读，并按稳定 ID 解析内容。
 */
void ULRDialogueSubsystem::InitializeContent(ULRGameContentSet* contentSet)
{
	ContentSet = contentSet;
	DialogueScriptRegistry = contentSet ? contentSet->DialogueScriptRegistry : nullptr;
	DialogueSpeakerRegistry = contentSet ? contentSet->DialogueSpeakerRegistry : nullptr;
	ResetSession();
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
	if (ActiveSUDSDialogue)
	{
		return AdvanceSUDSDialogue();
	}
	if (!HasActiveSession())
	{
		return Reject(NAME_None, LRGameplayTags::NarrativeRejectNoSession);
	}
	if (CurrentPage.SessionType == ELRNarrativeSessionType::Reading)
	{
		return FinishSession();
	}
	return Reject(CurrentPage.ContentId, LRGameplayTags::NarrativeRejectNoSession);
}

/**
 * @brief 执行 Select Choice 的纯规则或事务判定，失败时提供结构化原因。
 * @param choiceId 稳定标识 `choiceId`；用于内容查询和存档，不依赖显示名或数组序号。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FLRNarrativeResult ULRDialogueSubsystem::SelectChoice(const FName choiceId)
{
	if (ActiveSUDSDialogue)
	{
		const FLRNarrativeChoice* choice = CurrentPage.Choices.FindByPredicate([choiceId](const FLRNarrativeChoice& candidate)
		{
			return candidate.ChoiceId == choiceId;
		});
		return choice ? SelectSUDSChoice(choice->ChoiceIndex)
			: Reject(ActiveScriptId, LRGameplayTags::NarrativeRejectInvalidChoice);
	}
	return Reject(CurrentPage.ContentId, LRGameplayTags::NarrativeRejectNoSession);
}

/**
 * @brief 结束或取消 End Session 流程，并清理本次操作拥有的临时状态。
 */
void ULRDialogueSubsystem::EndSession()
{
	if (ActiveSUDSDialogue)
	{
		EndSUDSDialogue(ELRDialogueEndReason::Cancelled);
		return;
	}
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
	CurrentPage.LineText = CurrentPage.Text;
	CurrentPage.bShowSpeakerName = false;
	CurrentPage.bShowPortrait = false;
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
	const ELRNarrativeSessionType sessionType = CurrentPage.SessionType;
	const FName finalContentId = CurrentPage.ContentId;
	ResetSession();
	OnSessionEnded.Broadcast(sessionType, finalContentId);

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
}
