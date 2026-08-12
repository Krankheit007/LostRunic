/**
 * @file LRCollectiblePickupActor.h
 * @brief 收藏品拾取：配置 ULRCollectibleDefinition，AddCollectibleId 返回 Success 才隐藏世界 Actor；AlreadyOwned 保持 Actor 并记录明确诊断。
 *
 * 关联文件：LRCollectiblePickupActor.cpp；所属领域：Interaction。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Interaction/LRWorldInteractionActor.h"

#include "LRCollectiblePickupActor.generated.h"

class ULRCollectibleDefinition;

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(Blueprintable, meta = (DisplayName = "Lost Runic Collectible Pickup"))
class LOSTRUNIC_API ALRCollectiblePickupActor : public ALRWorldInteractionActor
{
	GENERATED_BODY()

public:
	/** Configures the default pickup action. */
	ALRCollectiblePickupActor();

	/** 拾取时写入收藏品集合的定义；CollectibleId 必须与定义资产一致。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Collectible")
	TObjectPtr<ULRCollectibleDefinition> CollectibleDefinition;

protected:
	/** 只有 AddCollectibleId 返回 Success 才隐藏 Actor 并标记一次性完成；AlreadyOwned 保持 Actor 可见。 */
	virtual FLRInteractionResult ExecuteInteractionInternal(AActor* interactor, FGameplayTag actionTag) override;

	/** Blueprint hook for a collectible pickup sound or effect; fired only after the record succeeds. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Lost Runic|Interaction")
	void OnCollectiblePickedUp();
};
