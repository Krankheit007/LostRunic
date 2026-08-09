/**
 * @file LRInteractionComponent.cpp
 * @brief 以可调计时器扫描交互候选，按照提示 500 cm、描边 200 cm、总计 90 度朝向、遮挡和状态选择唯一目标，并向 HUD 广播提示。
 *
 * 关联文件：LRInteractionComponent.h；所属领域：Interaction。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Interaction/LRInteractionComponent.h"

#include "Core/LRGameplayTags.h"
#include "Core/LRLog.h"
#include "Data/LRGameTuningSet.h"
#include "Data/LRInteractionTuning.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "Interaction/LRInteractable.h"
#include "Interaction/LRInteractionRules.h"
#include "Items/LRInventoryComponent.h"
#include "State/LRStateComponent.h"
#include "EngineUtils.h"
#include "TimerManager.h"

/**
 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
 */
ULRInteractionComponent::ULRInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

/**
 * @brief 在进入世界后解析运行时依赖、绑定事件并启动所需计时器；构造阶段不访问 World 或玩家对象。
 */
void ULRInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
	Inventory = GetOwner() ? GetOwner()->FindComponentByClass<ULRInventoryComponent>() : nullptr;
	State = GetOwner() ? GetOwner()->FindComponentByClass<ULRStateComponent>() : nullptr;
	const UGameInstance* gameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const ULRGameInstanceSubsystem* subsystem = gameInstance ? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	Tuning = subsystem && subsystem->GetTuningSet() ? subsystem->GetTuningSet()->Interaction : nullptr;
	if (!ensureMsgf(Inventory && State && Tuning, TEXT("%s requires Inventory, State, and Interaction tuning."), *GetNameSafe(this)))
	{
		return;
	}
	ScanCandidates();
	GetWorld()->GetTimerManager().SetTimer(QueryTimer, this, &ULRInteractionComponent::ScanCandidates,
		Tuning->QueryIntervalSeconds, true);
}

/**
 * @brief 解除委托并清理计时器或缓存，避免关卡切换和对象销毁后继续收到回调。
 * @param endPlayReason Unreal 提供的结束原因，用于区分销毁、关卡切换和退出。
 */
void ULRInteractionComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(QueryTimer);
	}
	Super::EndPlay(endPlayReason);
}

/**
 * @brief 对当前唯一候选重新校验距离、朝向和遮挡后执行主交互，并返回结构化结果。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FLRInteractionResult ULRInteractionComponent::PerformPrimaryInteraction()
{
	FLRInteractionResult result;
	result.ActionTag = CurrentOption.ActionTag;
	if (!CurrentTarget.IsValid())
	{
		result.FailureReason = LRGameplayTags::InteractionRejectNoTarget;
	}
	else if (CurrentRange != ELRInteractionRange::Executable)
	{
		result.FailureReason = LRGameplayTags::InteractionRejectTooFar;
	}
	else
	{
		result = ILRInteractable::Execute_ExecuteInteraction(CurrentTarget.Get(), GetOwner(), CurrentOption.ActionTag);
	}
	OnInteractionExecuted.Broadcast(result);
	return result;
}

/**
 * @brief 向对应日志分类输出当前状态、配置来源和关键运行时值，供 LR.Debug 命令诊断。
 */
void ULRInteractionComponent::LogDiagnostics() const
{
	UE_LOG(LogLostRunicInteraction, Display, TEXT("Owner=%s Target=%s Action=%s Range=%d"), *GetNameSafe(GetOwner()),
		*GetNameSafe(CurrentTarget.Get()), *CurrentOption.ActionTag.ToString(), static_cast<int32>(CurrentRange));
}

/**
 * @brief 按距离、朝向、遮挡和当前状态筛选交互候选，并只保留唯一最优目标。
 */
void ULRInteractionComponent::ScanCandidates()
{
	if (!GetWorld() || !Inventory || !State)
	{
		return;
	}

	TArray<FCandidate> candidates;
	TArray<FLRInteractionCandidateScore> scores;
	const FVector ownerLocation = GetOwner()->GetActorLocation();
	const FVector ownerForward = GetOwner()->GetActorForwardVector().GetSafeNormal2D();
	const FGameplayTagContainer ownedTags = Inventory->GetOwnedItemTags();
	for (TActorIterator<AActor> actorIt(GetWorld()); actorIt; ++actorIt)
	{
		AActor* actor = *actorIt;
		if (!actor || actor == GetOwner() || !actor->GetClass()->ImplementsInterface(ULRInteractable::StaticClass()))
		{
			continue;
		}
		const FVector targetLocation = ILRInteractable::Execute_GetInteractionLocation(actor);
		const FVector toTarget = targetLocation - ownerLocation;
		for (const FLRInteractionOption& option : ILRInteractable::Execute_GetInteractionOptions(actor, GetOwner()))
		{
			FCandidate candidate;
			candidate.Actor = actor;
			candidate.Option = option;
			candidate.Score.Distance = toTarget.Size2D();
			candidate.Score.ForwardDot = FVector::DotProduct(ownerForward, toTarget.GetSafeNormal2D());
			candidate.Score.bOccluded = IsOccluded(actor, targetLocation);
			candidate.Score.bModeAllowed = option.RequiredMode == State->GetCurrentMode();
			candidate.Score.bItemsAllowed = option.RequiredItemTags.IsEmpty() || option.RequiredItemTags.Matches(ownedTags);
			candidate.ExecuteDistance = option.MaxDistanceOverride > 0.0f
				? option.MaxDistanceOverride : GetEffectiveTuning().ExecuteDistance;
			candidates.Add(candidate);
			scores.Add(candidate.Score);
		}
	}
	ApplySelection(candidates, LRInteractionRules::SelectBestCandidate(scores, GetEffectiveTuning()));
}

/**
 * @brief 判断 Is Occluded 对应条件；不产生玩法副作用。
 * @param target 本次规则检查或操作的目标对象。
 * @param targetLocation 空间值 `targetLocation`；距离和位置使用 Unreal 厘米单位。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRInteractionComponent::IsOccluded(AActor* target, const FVector& targetLocation) const
{
	FHitResult hit;
	FCollisionQueryParams queryParams(SCENE_QUERY_STAT(LRInteractionOcclusion), false, GetOwner());
	const bool bHit = GetWorld()->LineTraceSingleByChannel(hit, GetOwner()->GetActorLocation(), targetLocation,
		ECC_Visibility, queryParams);
	return bHit && hit.GetActor() != target;
}

/**
 * @brief 把 Apply Selection 数据应用到运行时对象，并显式处理缺失依赖。
 * @param candidates 本次领域操作的结构化数据 `candidates`；字段语义由对应 USTRUCT 定义。
 * @param selectedIndex 本次操作使用的计数、增量或索引 `selectedIndex`；由函数校验合法范围。
 */
void ULRInteractionComponent::ApplySelection(const TArray<FCandidate>& candidates, const int32 selectedIndex)
{
	AActor* selectedTarget = candidates.IsValidIndex(selectedIndex) ? candidates[selectedIndex].Actor.Get() : nullptr;
	const FLRInteractionOption selectedOption = selectedTarget ? candidates[selectedIndex].Option : FLRInteractionOption();
	const ELRInteractionRange selectedRange = selectedTarget
		? LRInteractionRules::GetRange(candidates[selectedIndex].Score.Distance,
			candidates[selectedIndex].ExecuteDistance, GetEffectiveTuning()) : ELRInteractionRange::None;
	const bool bChanged = CurrentTarget.Get() != selectedTarget || CurrentOption.ActionTag != selectedOption.ActionTag
		|| CurrentRange != selectedRange;
	CurrentTarget = selectedTarget;
	CurrentOption = selectedOption;
	CurrentRange = selectedRange;
	if (bChanged)
	{
		OnTargetChanged.Broadcast(selectedTarget, CurrentOption, CurrentRange);
	}
}

/**
 * @brief 查询 Effective Tuning；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
const ULRInteractionTuning& ULRInteractionComponent::GetEffectiveTuning() const
{
	return Tuning ? *Tuning : *GetDefault<ULRInteractionTuning>();
}
