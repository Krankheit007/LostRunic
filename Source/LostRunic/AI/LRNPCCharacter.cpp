/**
 * @file LRNPCCharacter.cpp
 * @brief 通用非战斗 NPC 实现：对话交互（Talk 选项经 ULRDialogueSubsystem::StartDialogue）与噪声表现钩子；行为由 StateTree/控制器驱动。
 *
 * 关联文件：LRNPCCharacter.h；所属领域：AI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "AI/LRNPCCharacter.h"

#include "AI/LRNPCController.h"
#include "Core/LRGameplayTags.h"
#include "Core/LRLog.h"
#include "Data/LRNPCDefinition.h"
#include "Engine/GameInstance.h"
#include "Narrative/LRDialogueSubsystem.h"

/**
 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
 */
ALRNPCCharacter::ALRNPCCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	AIControllerClass = ALRNPCController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

/**
 * @brief 查询 Patrol Point；不修改领域状态。
 * @param index 目标元素索引，调用前必须满足对应容器边界。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
AActor* ALRNPCCharacter::GetPatrolPoint(const int32 index) const
{
	return PatrolPoints.IsValidIndex(index) ? PatrolPoints[index].Get() : nullptr;
}

/**
 * @brief 通知 NPC 听见噪声：触发表现钩子与预留委托；Conversation 高优先级时由控制器决定是否切换行为。
 * @param location 世界空间位置，Unreal 单位为厘米。
 * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
 */
void ALRNPCCharacter::NotifyNoiseHeard(const FVector location, const FGameplayTag reason)
{
	OnNoiseHeard(location, reason);
	OnNPCAttentionChanged.Broadcast(location, reason);
}

/**
 * @brief 查询当前 NPC 行为（由控制器权威解析）；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
ELRNPCBehaviorState ALRNPCCharacter::GetActiveBehavior() const
{
	const ALRNPCController* controller = Cast<ALRNPCController>(GetController());
	return controller ? controller->GetActiveBehavior() : ELRNPCBehaviorState::Idle;
}

/**
 * @brief 查询 Interaction Options：仅 Talk（对话），要求 Normal 状态。
 * @param interactor 参与本次操作的运行时对象 `interactor`；函数会检查空值和所需接口。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
TArray<FLRInteractionOption> ALRNPCCharacter::GetInteractionOptions_Implementation(AActor* interactor)
{
	TArray<FLRInteractionOption> options;
	if (Definition && !Definition->DialogueRowId.IsNone())
	{
		FLRInteractionOption option;
		option.ActionTag = LRGameplayTags::InteractionActionTalk;
		option.Prompt = NSLOCTEXT("LostRunic", "NPC.TalkPrompt", "对话");
		option.RequiredMode = ELRPerceptionMode::Normal;
		options.Add(option);
	}
	return options;
}

/**
 * @brief 查询 Interaction Location；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FVector ALRNPCCharacter::GetInteractionLocation_Implementation()
{
	return GetActorLocation();
}

/**
 * @brief 实现 Execute Interaction 对应的领域步骤：Talk 经 ULRDialogueSubsystem::StartDialogue 启动对话并进入 Conversation；对话结束回到默认行为。
 * @param interactor 参与本次操作的运行时对象 `interactor`；函数会检查空值和所需接口。
 * @param actionTag Gameplay Tag 或标签集合，用于分类、条件、拒绝原因和可诊断事件。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FLRInteractionResult ALRNPCCharacter::ExecuteInteraction_Implementation(AActor* interactor, const FGameplayTag actionTag)
{
	FLRInteractionResult result;
	result.ActionTag = actionTag;
	if (actionTag != LRGameplayTags::InteractionActionTalk)
	{
		result.FailureReason = LRGameplayTags::InteractionRejectState;
		return result;
	}
	ULRDialogueSubsystem* dialogue = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULRDialogueSubsystem>() : nullptr;
	if (!dialogue || !Definition || Definition->DialogueRowId.IsNone())
	{
		result.FailureReason = LRGameplayTags::NarrativeRejectMissingContent;
		return result;
	}
	const FLRNarrativeResult narrative = dialogue->StartDialogue(Definition->DialogueRowId);
	if (!narrative.bSuccess)
	{
		result.FailureReason = narrative.FailureReason;
		return result;
	}
	if (ALRNPCController* controller = Cast<ALRNPCController>(GetController()))
	{
		controller->NotifyDialogueStarted();
	}
	dialogue->OnSessionEnded.AddUniqueDynamic(this, &ALRNPCCharacter::HandleDialogueSessionEnded);
	result.bSuccess = true;
	return result;
}

/**
 * @brief 处理 Handle Dialogue Session Ended 事件，将引擎回调转换为对应领域状态更新。
 * @param sessionType 本次操作使用的 `sessionType` 枚举或模式值。
 * @param contentId 稳定标识 `contentId`；用于内容查询和存档，不依赖显示名或数组序号。
 */
void ALRNPCCharacter::HandleDialogueSessionEnded(const ELRNarrativeSessionType sessionType, const FName contentId)
{
	if (ALRNPCController* controller = Cast<ALRNPCController>(GetController()))
	{
		controller->NotifyDialogueEnded();
	}
	if (UGameInstance* gameInstance = GetGameInstance())
	{
		if (ULRDialogueSubsystem* dialogue = gameInstance->GetSubsystem<ULRDialogueSubsystem>())
		{
			dialogue->OnSessionEnded.RemoveDynamic(this, &ALRNPCCharacter::HandleDialogueSessionEnded);
		}
	}
}
