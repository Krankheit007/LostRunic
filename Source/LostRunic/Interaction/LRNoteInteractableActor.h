/**
 * @file LRNoteInteractableActor.h
 * @brief 可重复阅读的笔记交互：配置稳定 ReadingId，阅读会话成功打开时立即记录笔记 ID；重复打开不产生重复记录。
 *
 * 关联文件：LRNoteInteractableActor.cpp；所属领域：Interaction。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Interaction/LRWorldInteractionActor.h"

#include "LRNoteInteractableActor.generated.h"

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(Blueprintable, meta = (DisplayName = "Lost Runic Note Interactable"))
class LOSTRUNIC_API ALRNoteInteractableActor : public ALRWorldInteractionActor
{
	GENERATED_BODY()

public:
	/** Configures the default read action and repeatable completion. */
	ALRNoteInteractableActor();

	/** 阅读内容的稳定 ReadingId；必须与 ReadingTable 行名一致。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Note")
	FName ReadingId = NAME_None;

protected:
	/** 阅读会话成功打开时立即 AddNoteId(ReadingId)；文本展示仍由叙事子系统与阅读 UI 负责。 */
	virtual FLRInteractionResult ExecuteInteractionInternal(AActor* interactor, FGameplayTag actionTag) override;
};
