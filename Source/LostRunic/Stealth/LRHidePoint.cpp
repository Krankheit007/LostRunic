/**
 * @file LRHidePoint.cpp
 * @brief 实现固定/可移动躲藏点、守卫可见性接口和统一噪声发布，使守卫通过事件感知玩家而非轮询角色速度或修改基础视野。
 *
 * 关联文件：LRHidePoint.h；所属领域：Stealth。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Stealth/LRHidePoint.h"

#include "Components/SceneComponent.h"
#include "Core/LRGameplayTags.h"
#include "Stealth/LRHideComponent.h"

/**
 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
 */
ALRHidePoint::ALRHidePoint()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	InteractionOption.ActionTag = LRGameplayTags::InteractionActionHide;
	InteractionOption.Prompt = NSLOCTEXT("LostRunic", "HidePrompt", "Hide");
}

/**
 * @brief 查询 Interaction Options_Implementation；不修改领域状态。
 * @param interactor 参与本次操作的运行时对象 `interactor`；函数会检查空值和所需接口。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
TArray<FLRInteractionOption> ALRHidePoint::GetInteractionOptions_Implementation(AActor* interactor)
{
	return { InteractionOption };
}

/**
 * @brief 查询 Interaction Location_Implementation；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FVector ALRHidePoint::GetInteractionLocation_Implementation()
{
	return GetActorLocation();
}

/** Uses the actor root as the fallback prompt anchor for hide points. */
USceneComponent* ALRHidePoint::GetInteractionPromptAnchorComponent_Implementation()
{
	return GetRootComponent();
}

/**
 * @brief 执行当前交互选项；物品目标仍通过统一物品事务入口结算。
 * @param interactor 参与本次操作的运行时对象 `interactor`；函数会检查空值和所需接口。
 * @param actionTag Gameplay Tag 或标签集合，用于分类、条件、拒绝原因和可诊断事件。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FLRInteractionResult ALRHidePoint::ExecuteInteraction_Implementation(AActor* interactor, const FGameplayTag actionTag)
{
	FLRInteractionResult result;
	result.ActionTag = actionTag;
	ULRHideComponent* hide = interactor ? interactor->FindComponentByClass<ULRHideComponent>() : nullptr;
	if (!hide)
	{
		result.FailureReason = LRGameplayTags::InteractionRejectState;
		return result;
	}
	result.bSuccess = hide->GetCurrentHidePoint() == this ? hide->ExitHidePoint() : hide->EnterHidePoint(this);
	if (!result.bSuccess)
	{
		result.FailureReason = LRGameplayTags::InteractionRejectState;
	}
	return result;
}
