/**
 * @file LRDialogueSubsystem.h
 * @brief 驱动 SUDS Dialogue 与 Reading DataTable 会话，记录剧情状态，并向 UI 发布当前台词、阅读内容及结束事件。
 *
 * 关联文件：LRDialogueSubsystem.cpp；所属领域：Narrative。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Narrative/LRNarrativeTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Save/LRSaveV2Types.h"

#include "LRDialogueSubsystem.generated.h"

class USUDSDialogue;
class USUDSScript;
class ULRDialogueEventBridge;
class ULRDialogueStateParticipant;
class ULRDialogueScriptRegistry;
class ULRDialogueSpeakerRegistry;
class ULRGameContentSet;
class ULRLevelEventDefinition;

UENUM(BlueprintType)
enum class ELRDialogueEndReason : uint8
{
	None,
	CompletedNaturally,
	Cancelled,
	OwnerDestroyed,
	LevelTravel,
	Rejected,
	Error
};

USTRUCT(BlueprintType)
struct LOSTRUNIC_API FLRDialogueStartRequest
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Dialogue")
	FName ScriptId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category="Dialogue")
	TObjectPtr<USUDSScript> Script = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Dialogue")
	FName StartLabel = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category="Dialogue")
	TWeakObjectPtr<UObject> Owner;

	UPROPERTY(BlueprintReadOnly, Category="Dialogue")
	FGameplayTag CompletionStoryTag;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRNarrativePageChanged, FLRNarrativePage, page);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLRNarrativeSessionEnded, ELRNarrativeSessionType, sessionType,
	FName, finalContentId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLRNarrativeEventCommitted, FName, eventId,
	ELRSavePolicy, savePolicy);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLRNarrativeRequestRejected, FName, contentId,
	FGameplayTag, reason);

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(meta = (DisplayName = "Lost Runic Dialogue Subsystem"))
class LOSTRUNIC_API ULRDialogueSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * @brief 初始化子系统拥有的长期状态与事件绑定。
	 * @param collection 调用方提供的 `collection`，只在本次操作范围内使用。
	 */
	virtual void Initialize(FSubsystemCollectionBase& collection) override;
	/**
	 * @brief 释放子系统事件绑定和运行时缓存。
	 */
	virtual void Deinitialize() override;

	/**
	 * @brief 从 LRGameContentSet 建立 DataTable 与定义资产索引，缺失内容返回可诊断错误。
	 * @param contentSet 数据或调优来源 `contentSet`；调用期间只读，并按稳定 ID 解析内容。
	 */
	void InitializeContent(ULRGameContentSet* contentSet);

	/** SUDS-backed production dialogue entry point. */
	FLRNarrativeResult StartSUDSDialogue(const FLRDialogueStartRequest& request);
	/** Returns the project-level Registry used to resolve every dialogue ScriptId. */
	const ULRDialogueScriptRegistry* GetDialogueScriptRegistry() const { return DialogueScriptRegistry; }
	FLRNarrativeResult AdvanceSUDSDialogue();
	FLRNarrativeResult SelectSUDSChoice(int32 choiceIndex);
	void EndSUDSDialogue(ELRDialogueEndReason reason, UObject* owner = nullptr);
	FName GetActiveScriptId() const { return ActiveScriptId; }

	/**
	 * @brief 使用稳定 Reading 行 ID 启动阅读会话，并复用叙事推进规则。
	 * @param readingId 稳定标识 `readingId`；用于内容查询和存档，不依赖显示名或数组序号。
	 * @param completionEventId 稳定标识 `completionEventId`；用于内容查询和存档，不依赖显示名或数组序号。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Narrative")
	FLRNarrativeResult StartReading(FName readingId, FName completionEventId = NAME_None);

	/**
	 * @brief 推进当前对话或阅读会话；根据条件选择下一行并处理一次性事件。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Narrative")
	FLRNarrativeResult Advance();

	/**
	 * @brief 执行 Select Choice 的纯规则或事务判定，失败时提供结构化原因。
	 * @param choiceId 稳定标识 `choiceId`；用于内容查询和存档，不依赖显示名或数组序号。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Narrative")
	FLRNarrativeResult SelectChoice(FName choiceId);

	/**
	 * @brief 结束或取消 End Session 流程，并清理本次操作拥有的临时状态。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Narrative")
	void EndSession();

	/**
	 * @brief 检查关卡事件条件和一次性标记，成功后提交剧情进度及其显式存档策略。
	 * @param eventId 剧情事件的稳定 FName ID，用于一次性判定和存档。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Narrative|Events")
	FLRNarrativeResult TryCompleteEvent(FName eventId);

	/**
	 * @brief 更新 Context Tags，并在需要时同步组件状态或广播变化事件。
	 * @param contextTags Gameplay Tag 或标签集合，用于分类、条件、拒绝原因和可诊断事件。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Narrative|Conditions")
	void SetContextTags(const FGameplayTagContainer& contextTags);

	/**
	 * @brief 查询 Context Tags；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Narrative|Conditions")
	FGameplayTagContainer GetContextTags() const { return ContextTags; }

	/**
	 * @brief 判断 Has Active Session 对应条件；不产生玩法副作用。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Narrative")
	bool HasActiveSession() const { return CurrentPage.SessionType != ELRNarrativeSessionType::None; }

	/**
	 * @brief 查询 Current Page；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Narrative")
	FLRNarrativePage GetCurrentPage() const { return CurrentPage; }

	/**
	 * @brief 判断 Is Event Completed 对应条件；不产生玩法副作用。
	 * @param eventId 剧情事件的稳定 FName ID，用于一次性判定和存档。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Narrative|Events")
	bool IsEventCompleted(FName eventId) const { return CompletedEventIds.Contains(eventId); }

	/**
	 * @brief 把 Restore Completed Events 数据应用到运行时对象，并显式处理缺失依赖。
	 * @param eventIds 调用方提供的 `eventIds`，只在本次操作范围内使用。
	 */
	void RestoreCompletedEvents(const TSet<FName>& eventIds);
	/**
	 * @brief 查询 Completed Events；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	const TSet<FName>& GetCompletedEvents() const { return CompletedEventIds; }

	/** V2 save adapter. Memory events remain a separate durable chunk. */
	void CaptureStorySaveState(FLRSaveStoryChunk& outStory) const;
	void RestoreStorySaveState(const FLRSaveStoryChunk& savedStory);
	void CaptureMemoryEventIds(TSet<FName>& outEventIds) const;
	void RestoreMemoryEventIds(const TSet<FName>& eventIds);
	bool RecordMemoryEvent(FName eventId);
	/** Clears transient narrative presentation state when a New Game begins. */
	void ResetForNewGame();

	/** 当 Page Changed 发生时广播；蓝图可绑定该委托以更新表现，不应在回调中改写核心规则。  */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Narrative")
	FLRNarrativePageChanged OnPageChanged;

	/** 当 Session Ended 发生时广播；蓝图可绑定该委托以更新表现，不应在回调中改写核心规则。  */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Narrative")
	FLRNarrativeSessionEnded OnSessionEnded;

	/** 当 Event Committed 发生时广播；蓝图可绑定该委托以更新表现，不应在回调中改写核心规则。  */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Narrative|Events")
	FLRNarrativeEventCommitted OnEventCommitted;

	/** 当 Request Rejected 发生时广播；蓝图可绑定该委托以更新表现，不应在回调中改写核心规则。  */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Narrative")
	FLRNarrativeRequestRejected OnRequestRejected;

private:
	/**
	 * @brief 解析指定阅读行并发布给 UI，同时记录稳定笔记 ID。
	 * @param readingId 稳定标识 `readingId`；用于内容查询和存档，不依赖显示名或数组序号。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	FLRNarrativeResult ShowReadingRow(FName readingId);
	/**
	 * @brief 结束当前对话或阅读会话，提交最终事件并广播会话结束。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	FLRNarrativeResult FinishSession();
	/**
	 * @brief 创建带原因 Gameplay Tag 的结构化失败结果，并保留事务不变量。
	 * @param contentId 稳定标识 `contentId`；用于内容查询和存档，不依赖显示名或数组序号。
	 * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	FLRNarrativeResult Reject(FName contentId, FGameplayTag reason);
	/**
	 * @brief 按稳定 ID 或运行时条件查找 Event Definition，未找到时返回明确失败值。
	 * @param eventId 剧情事件的稳定 FName ID，用于一次性判定和存档。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	ULRLevelEventDefinition* FindEventDefinition(FName eventId) const;
	/**
	 * @brief 清空当前对话/阅读行、选项和表现状态，但保留已完成剧情事件。
	 */
	void ResetSession();
	UFUNCTION()
	void HandleSUDSSpeakerLine(USUDSDialogue* dialogue);
	UFUNCTION()
	void HandleSUDSChoice(USUDSDialogue* dialogue, int choiceIndex);
	UFUNCTION()
	void HandleSUDSFinished(USUDSDialogue* dialogue);
	void FinishSUDSSession(ELRDialogueEndReason reason);

	/** Content Set 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TObjectPtr<ULRGameContentSet> ContentSet;

	UPROPERTY(Transient)
	TObjectPtr<ULRDialogueScriptRegistry> DialogueScriptRegistry;

	UPROPERTY(Transient)
	TObjectPtr<ULRDialogueSpeakerRegistry> DialogueSpeakerRegistry;

	/** Current Page 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	FLRNarrativePage CurrentPage;

	/** Context Tags 的 Gameplay Tag 条件或分类，用于数据驱动规则与诊断。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	FGameplayTagContainer ContextTags;

	/** Completed Event Ids 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TSet<FName> CompletedEventIds;

	UPROPERTY(Transient)
	TSet<FName> MemoryEventIds;

	UPROPERTY(Transient)
	TObjectPtr<USUDSDialogue> ActiveSUDSDialogue;

	UPROPERTY(Transient)
	TObjectPtr<USUDSScript> ActiveSUDSScript;

	UPROPERTY(Transient)
	TObjectPtr<ULRDialogueEventBridge> DialogueEventBridge;

	UPROPERTY(Transient)
	TObjectPtr<ULRDialogueStateParticipant> DialogueStateParticipant;

	TWeakObjectPtr<UObject> ActiveDialogueOwner;
	FName ActiveScriptId = NAME_None;
	FGameplayTag ActiveCompletionStoryTag;
	ELRDialogueEndReason ActiveEndReason = ELRDialogueEndReason::None;
};
