/**
 * @file LRItemUseTargetActor.cpp
 * @brief 实现 4 格快捷栏、背包、笔记、收藏品和统一物品使用事务；快捷栏与交互后选物共用解析入口，失败时回滚消耗并返回结构化原因。
 *
 * 关联文件：LRItemUseTargetActor.h；所属领域：Items。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Items/LRItemUseTargetActor.h"

#include "Core/LRGameplayTags.h"
#include "Data/LRItemDefinition.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Narrative/LRDialogueSubsystem.h"

/**
 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
 */
ALRItemUseTargetActor::ALRItemUseTargetActor()
{
	PrimaryActorTick.bCanEverTick = false;
	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	SetRootComponent(VisualMesh);
	VisualMesh->SetCollisionProfileName(TEXT("BlockAll"));
	InteractionOption.ActionTag = LRGameplayTags::InteractionActionUse;
}

/**
 * @brief 查询 Interaction Options_Implementation；不修改领域状态。
 * @param interactor 参与本次操作的运行时对象 `interactor`；函数会检查空值和所需接口。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
TArray<FLRInteractionOption> ALRItemUseTargetActor::GetInteractionOptions_Implementation(AActor* interactor)
{
	return bCompleted && bOneShot ? TArray<FLRInteractionOption>() : TArray<FLRInteractionOption>({ InteractionOption });
}

/**
 * @brief 查询 Interaction Location_Implementation；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FVector ALRItemUseTargetActor::GetInteractionLocation_Implementation()
{
	return GetActorLocation();
}

/**
 * @brief 执行当前交互选项；物品目标仍通过统一物品事务入口结算。
 * @param interactor 参与本次操作的运行时对象 `interactor`；函数会检查空值和所需接口。
 * @param actionTag Gameplay Tag 或标签集合，用于分类、条件、拒绝原因和可诊断事件。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FLRInteractionResult ALRItemUseTargetActor::ExecuteInteraction_Implementation(AActor* interactor,
	const FGameplayTag actionTag)
{
	FLRInteractionResult result;
	result.ActionTag = actionTag;
	result.FailureReason = bCompleted && bOneShot ? LRGameplayTags::InteractionRejectCompleted
		: LRGameplayTags::InteractionRejectItem;
	return result;
}

/**
 * @brief 查询 Item Use Target Tags_Implementation；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FGameplayTagContainer ALRItemUseTargetActor::GetItemUseTargetTags_Implementation()
{
	return TargetTags;
}

/**
 * @brief 把 Apply Item Use_Implementation 数据应用到运行时对象，并显式处理缺失依赖。
 * @param request 不可变领域请求，包含本次操作所需的稳定 ID、来源、目标或原因。
 * @param definition 数据或调优来源 `definition`；调用期间只读，并按稳定 ID 解析内容。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FLRItemUseResult ALRItemUseTargetActor::ApplyItemUse_Implementation(const FLRItemUseRequest& request,
	ULRItemDefinition* definition)
{
	FLRItemUseResult result;
	result.ItemId = request.ItemId;
	if (bCompleted && bOneShot)
	{
		result.FailureReason = LRGameplayTags::InteractionRejectCompleted;
		return result;
	}
	bCompleted = true;
	result.bSuccess = true;
	result.EventId = EventId;
	if (!EventId.IsNone())
	{
		UGameInstance* gameInstance = GetGameInstance();
		ULRDialogueSubsystem* dialogue = gameInstance ? gameInstance->GetSubsystem<ULRDialogueSubsystem>() : nullptr;
		if (dialogue)
		{
			dialogue->TryCompleteEvent(EventId);
		}
	}
	if (bHideAfterUse)
	{
		SetActorHiddenInGame(true);
		SetActorEnableCollision(false);
	}
	OnItemUseApplied(request, definition);
	return result;
}
