/**
 * @file LRDialogueWidgetController.cpp
 * @brief 实现 HUD、状态遮罩、对话/阅读、背包/笔记/收藏、暂停、存档槽和过场的控制器边界。UI 订阅领域事件并负责表现，不参与核心规则判定。
 *
 * 关联文件：LRDialogueWidgetController.h；所属领域：UI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "UI/LRDialogueWidgetController.h"

#include "Data/LRUITuning.h"
#include "Engine/World.h"
#include "Narrative/LRDialogueSubsystem.h"
#include "TimerManager.h"

/**
 * @brief 初始化子系统拥有的长期状态与事件绑定。
 * @param dialogueSubsystem 调用方提供的 `dialogueSubsystem`，只在本次操作范围内使用。
 * @param tuning 数据或调优来源 `tuning`；调用期间只读，并按稳定 ID 解析内容。
 * @param world 要解析地图 ID、应用恢复状态或执行查询的 Unreal World。
 */
void ULRDialogueWidgetController::Initialize(ULRDialogueSubsystem* dialogueSubsystem, ULRUITuning* tuning, UWorld* world)
{
	Deinitialize();
	DialogueSubsystem = dialogueSubsystem;
	Tuning = tuning;
	World = world;
	if (DialogueSubsystem)
	{
		DialogueSubsystem->OnPageChanged.AddDynamic(this, &ULRDialogueWidgetController::HandlePageChanged);
		DialogueSubsystem->OnSessionEnded.AddDynamic(this, &ULRDialogueWidgetController::HandleSessionEnded);
	}
}

/**
 * @brief 释放子系统事件绑定和运行时缓存。
 */
void ULRDialogueWidgetController::Deinitialize()
{
	StopTypewriter();
	if (DialogueSubsystem)
	{
		DialogueSubsystem->OnPageChanged.RemoveDynamic(this, &ULRDialogueWidgetController::HandlePageChanged);
		DialogueSubsystem->OnSessionEnded.RemoveDynamic(this, &ULRDialogueWidgetController::HandleSessionEnded);
	}
	DialogueSubsystem = nullptr;
	Tuning = nullptr;
	World.Reset();
	Presentation = FLRNarrativePresentation();
	FullText.Reset();
}

/**
 * @brief 处理 Handle Confirm 事件，将引擎回调转换为对应领域状态更新。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FLRNarrativeResult ULRDialogueWidgetController::HandleConfirm()
{
	if (!DialogueSubsystem || !DialogueSubsystem->HasActiveSession())
	{
		return FLRNarrativeResult();
	}
	if (!Presentation.bTextFullyRevealed)
	{
		RevealCurrentText();
		FLRNarrativeResult result;
		result.bSuccess = true;
		result.Action = ELRNarrativeAction::RevealCurrentText;
		result.ContentId = Presentation.Page.ContentId;
		return result;
	}
	return DialogueSubsystem->Advance();
}

/**
 * @brief 执行 Select Choice 的纯规则或事务判定，失败时提供结构化原因。
 * @param choiceId 稳定标识 `choiceId`；用于内容查询和存档，不依赖显示名或数组序号。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FLRNarrativeResult ULRDialogueWidgetController::SelectChoice(const FName choiceId)
{
	return DialogueSubsystem ? DialogueSubsystem->SelectChoice(choiceId) : FLRNarrativeResult();
}

/**
 * @brief 结束或取消 End Session 流程，并清理本次操作拥有的临时状态。
 */
void ULRDialogueWidgetController::EndSession()
{
	if (DialogueSubsystem)
	{
		DialogueSubsystem->EndSession();
	}
}

/**
 * @brief 立即显示当前叙事页面全文；下一次确认才推进到下一行。
 */
void ULRDialogueWidgetController::RevealCurrentText()
{
	UpdateVisibleCharacters(FullText.Len());
	StopTypewriter();
}

/**
 * @brief 实现 Advance Typewriter For Test 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 * @param elapsedSeconds 时间值 `elapsedSeconds`，单位为秒。
 */
void ULRDialogueWidgetController::AdvanceTypewriterForTest(const float elapsedSeconds)
{
	UpdateVisibleCharacters(FMath::FloorToInt(FMath::Max(0.0f, elapsedSeconds) * GetCharactersPerSecond()));
}

/**
 * @brief 处理 Handle Page Changed 事件，将引擎回调转换为对应领域状态更新。
 * @param page 本次领域操作的结构化数据 `page`；字段语义由对应 USTRUCT 定义。
 */
void ULRDialogueWidgetController::HandlePageChanged(const FLRNarrativePage page)
{
	StopTypewriter();
	Presentation = FLRNarrativePresentation();
	Presentation.Page = page;
	FullText = page.Text.ToString();
	PageStartedAtSeconds = World.IsValid() ? World->GetRealTimeSeconds() : 0.0;
	UpdateVisibleCharacters(0);
	if (FullText.IsEmpty())
	{
		return;
	}
	if (World.IsValid())
	{
		World->GetTimerManager().SetTimer(TypewriterTimer, this, &ULRDialogueWidgetController::RefreshTypewriter,
			GetUpdateSeconds(), true);
	}
}

/**
 * @brief 处理 Handle Session Ended 事件，将引擎回调转换为对应领域状态更新。
 * @param sessionType 本次操作使用的 `sessionType` 枚举或模式值。
 * @param finalContentId 稳定标识 `finalContentId`；用于内容查询和存档，不依赖显示名或数组序号。
 */
void ULRDialogueWidgetController::HandleSessionEnded(const ELRNarrativeSessionType sessionType, const FName finalContentId)
{
	StopTypewriter();
	Presentation = FLRNarrativePresentation();
	FullText.Reset();
	OnPresentationChanged.Broadcast(Presentation);
}

/**
 * @brief 根据 UI 调优速度刷新当前可见字符数；该计时器仅控制表现，不决定叙事推进。
 */
void ULRDialogueWidgetController::RefreshTypewriter()
{
	if (!World.IsValid())
	{
		StopTypewriter();
		return;
	}
	const double elapsedSeconds = World->GetRealTimeSeconds() - PageStartedAtSeconds;
	UpdateVisibleCharacters(FMath::FloorToInt(static_cast<float>(elapsedSeconds) * GetCharactersPerSecond()));
}

/**
 * @brief 根据最新领域状态刷新 Update Visible Characters，并仅在值变化时通知订阅者。
 * @param visibleCharacterCount 本次操作使用的计数、增量或索引 `visibleCharacterCount`；由函数校验合法范围。
 */
void ULRDialogueWidgetController::UpdateVisibleCharacters(const int32 visibleCharacterCount)
{
	const int32 clampedCount = FMath::Clamp(visibleCharacterCount, 0, FullText.Len());
	const bool bWasFullyRevealed = Presentation.bTextFullyRevealed;
	Presentation.DisplayedText = FText::FromString(FullText.Left(clampedCount));
	Presentation.bTextFullyRevealed = clampedCount == FullText.Len();
	OnPresentationChanged.Broadcast(Presentation);
	if (Presentation.bTextFullyRevealed && !bWasFullyRevealed)
	{
		StopTypewriter();
	}
}

/**
 * @brief 结束或取消 Stop Typewriter 流程，并清理本次操作拥有的临时状态。
 */
void ULRDialogueWidgetController::StopTypewriter()
{
	if (World.IsValid())
	{
		World->GetTimerManager().ClearTimer(TypewriterTimer);
	}
}

/**
 * @brief 查询 Characters Per Second；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
float ULRDialogueWidgetController::GetCharactersPerSecond() const
{
	return Tuning ? Tuning->TypewriterCharactersPerSecond : 30.0f;
}

/**
 * @brief 查询 Update Seconds；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
float ULRDialogueWidgetController::GetUpdateSeconds() const
{
	return Tuning ? Tuning->TypewriterUpdateSeconds : 0.033f;
}
