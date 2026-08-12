/**
 * @file LRNoteInteractableActor.cpp
 * @brief 可重复阅读的笔记交互：阅读会话成功打开时立即记录笔记 ID；重复打开不产生重复记录。
 *
 * 关联文件：LRNoteInteractableActor.h；所属领域：Interaction。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Interaction/LRNoteInteractableActor.h"

#include "Core/LRGameplayTags.h"
#include "Core/LRLog.h"
#include "Framework/LRCharacter.h"
#include "Items/LRInventoryComponent.h"
#include "Narrative/LRDialogueSubsystem.h"

/** Sets the content-default read action; notes stay readable after being recorded. */
ALRNoteInteractableActor::ALRNoteInteractableActor()
{
	FLRInteractionOption option;
	option.ActionTag = LRGameplayTags::InteractionActionRead;
	InteractionOptions = { option };
	bOneShot = false;
}

/**
 * @brief 阅读会话成功打开时立即 AddNoteId(ReadingId)；文本展示仍由叙事子系统与阅读 UI 负责。
 * @param interactor 参与本次操作的运行时对象 `interactor`；函数会检查空值和所需接口。
 * @param actionTag Gameplay Tag 或标签集合，用于分类、条件、拒绝原因和可诊断事件。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FLRInteractionResult ALRNoteInteractableActor::ExecuteInteractionInternal(AActor* interactor,
	const FGameplayTag actionTag)
{
	FLRInteractionResult result;
	result.ActionTag = actionTag;
	if (ReadingId.IsNone())
	{
		UE_LOG(LogLostRunicInteraction, Warning, TEXT("Note=%s has no ReadingId."), *GetNameSafe(this));
		result.FailureReason = LRGameplayTags::ItemUseRejectInvalidDefinition;
		return result;
	}
	const ALRCharacter* character = Cast<ALRCharacter>(interactor);
	ULRInventoryComponent* inventory = character ? character->GetInventoryComponent() : nullptr;
	ULRDialogueSubsystem* dialogueSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<ULRDialogueSubsystem>() : nullptr;
	if (!inventory || !dialogueSubsystem)
	{
		result.FailureReason = LRGameplayTags::ItemUseRejectExecution;
		return result;
	}

	const FLRNarrativeResult narrative = dialogueSubsystem->StartReading(ReadingId);
	if (!narrative.bSuccess)
	{
		result.FailureReason = narrative.FailureReason.IsValid()
			? narrative.FailureReason : LRGameplayTags::ItemUseRejectExecution;
		return result;
	}
	// AddNoteId 返回 false 表示笔记已存在：重复阅读是正常行为，交互仍整体成功，不产生 Warning。
	if (!inventory->AddNoteId(ReadingId))
	{
		UE_LOG(LogLostRunicInteraction, Verbose, TEXT("Note=%s reading=%s already recorded; repeat reading stays successful."),
			*GetNameSafe(this), *ReadingId.ToString());
	}
	result.bSuccess = true;
	return result;
}
