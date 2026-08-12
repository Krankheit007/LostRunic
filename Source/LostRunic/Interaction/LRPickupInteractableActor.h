/**
 * @file LRPickupInteractableActor.h
 * @brief 可拾取物品交互：配置 ULRItemDefinition 与拾取数量，库存 AddItem 成功后 Actor 才消失并标记完成；背包已满或非法配置时保持可见并返回结构化原因。
 *
 * 关联文件：LRPickupInteractableActor.cpp；所属领域：Interaction。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Interaction/LRWorldInteractionActor.h"

#include "LRPickupInteractableActor.generated.h"

class ULRItemDefinition;

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(Blueprintable, meta = (DisplayName = "Lost Runic Pickup Interactable"))
class LOSTRUNIC_API ALRPickupInteractableActor : public ALRWorldInteractionActor
{
	GENERATED_BODY()

public:
	/** Configures the default pickup action. */
	ALRPickupInteractableActor();

	/** 拾取时写入库存的物品定义；ItemId 必须与定义资产一致。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<ULRItemDefinition> ItemDefinition;

	/** 单次拾取数量；必须为正数，且不超过定义的 MaxStackSize。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup", meta = (ClampMin = "1", ClampMax = "999"))
	int32 PickupQuantity = 1;

protected:
	/** 只有库存 AddItem 返回 Success 才隐藏 Actor、关闭碰撞并标记一次性完成。 */
	virtual FLRInteractionResult ExecuteInteractionInternal(AActor* interactor, FGameplayTag actionTag) override;

	/** Blueprint hook for a pickup sound or temporary visual effect; fired only after the inventory transaction succeeds. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Lost Runic|Interaction")
	void OnPickupCompleted();
};
