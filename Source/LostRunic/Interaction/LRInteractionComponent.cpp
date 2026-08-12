/**
 * @file LRInteractionComponent.cpp
 * @brief 扫描 Interaction 通道，分离计算世界表现与执行资格，并向 HUD 发布唯一焦点。
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
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "Framework/LRPlayerController.h"
#include "Input/LRInputConfig.h"
#include "Interaction/LRInteractable.h"
#include "Interaction/LRInteractionPresentationComponent.h"
#include "Interaction/LRInteractionRules.h"
#include "Items/LRInventoryComponent.h"
#include "State/LRStateComponent.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
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
	RefreshInteractionState();
	return result;
}

/** Forces an immediate scan after interaction success so stale Focus prompts never linger. */
void ULRInteractionComponent::RefreshInteractionState()
{
	ScanCandidates();
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

	TArray<FEvaluation> evaluations;
	BuildEvaluations(evaluations);
	const int32 focusedIndex = SelectFocusedEvaluation(evaluations);
	ApplyPresentationStates(evaluations, focusedIndex);
	ApplySelection(evaluations, focusedIndex);
}

/** Queries only the configured Interaction object channel and derives side-effect-free evaluations. */
void ULRInteractionComponent::BuildEvaluations(TArray<FEvaluation>& outEvaluations) const
{
	const ULRInteractionTuning& tuning = GetEffectiveTuning();
	const FVector ownerLocation = GetOwner()->GetActorLocation();
	const FVector ownerForward = GetOwner()->GetActorForwardVector().GetSafeNormal2D();
	const FGameplayTagContainer ownedTags = Inventory->GetOwnedItemTags();
	TArray<FOverlapResult> overlaps;
	FCollisionQueryParams queryParams(SCENE_QUERY_STAT(LRInteractionQuery), false, GetOwner());
	const FCollisionObjectQueryParams objectParams(ECC_GameTraceChannel1);
	GetWorld()->OverlapMultiByObjectType(overlaps, ownerLocation, FQuat::Identity, objectParams,
		FCollisionShape::MakeSphere(tuning.FarHintDistance), queryParams);

	TSet<TWeakObjectPtr<AActor>> processedActors;
	for (const FOverlapResult& overlap : overlaps)
	{
		AActor* actor = overlap.GetActor();
		if (!actor || actor == GetOwner() || processedActors.Contains(actor)
			|| !actor->GetClass()->ImplementsInterface(ULRInteractable::StaticClass()))
		{
			continue;
		}
		processedActors.Add(actor);
		const FVector targetLocation = ILRInteractable::Execute_GetInteractionLocation(actor);
		const FVector toTarget = targetLocation - ownerLocation;
		const float distanceSquared = FVector::DistSquared(ownerLocation, targetLocation);
		const ELRInteractionPresentationState presentationState = LRInteractionRules::GetPresentationState(distanceSquared, tuning);
		for (const FLRInteractionOption& option : ILRInteractable::Execute_GetInteractionOptions(actor, GetOwner()))
		{
			FEvaluation& evaluation = outEvaluations.AddDefaulted_GetRef();
			evaluation.Actor = actor;
			evaluation.Option = option;
			evaluation.Score.DistanceSquared = distanceSquared;
			evaluation.Score.ForwardDot = FVector::DotProduct(ownerForward, toTarget.GetSafeNormal2D());
			evaluation.Score.bModeAllowed = option.RequiredMode == State->GetCurrentMode();
			evaluation.Score.bItemsAllowed = option.RequiredItemTags.IsEmpty() || option.RequiredItemTags.Matches(ownedTags);
			evaluation.ExecuteDistance = option.MaxDistanceOverride > 0.0f ? option.MaxDistanceOverride : tuning.ExecuteDistance;
			evaluation.bShowHint = presentationState != ELRInteractionPresentationState::None;
			evaluation.bCanExecute = evaluation.Score.bModeAllowed && evaluation.Score.bItemsAllowed
				&& LRInteractionRules::IsWithinExecutionDistance(distanceSquared, evaluation.ExecuteDistance);
			evaluation.PresentationState = presentationState;
		}
	}
}

/** Tests only executable, facing candidates for occlusion and returns the nearest remaining target. */
int32 ULRInteractionComponent::SelectFocusedEvaluation(TArray<FEvaluation>& evaluations) const
{
	TArray<int32> candidateIndices;
	for (int32 index = 0; index < evaluations.Num(); ++index)
	{
		const FEvaluation& evaluation = evaluations[index];
		if (evaluation.bCanExecute && LRInteractionRules::IsFacingAllowed(evaluation.Score.ForwardDot, GetEffectiveTuning()))
		{
			candidateIndices.Add(index);
		}
	}
	candidateIndices.Sort([&evaluations](const int32 left, const int32 right)
	{
		return evaluations[left].Score.DistanceSquared < evaluations[right].Score.DistanceSquared;
	});
	for (const int32 index : candidateIndices)
	{
		FEvaluation& evaluation = evaluations[index];
		AActor* actor = evaluation.Actor.Get();
		evaluation.Score.bOccluded = !actor || IsOccluded(actor, ILRInteractable::Execute_GetInteractionLocation(actor));
		if (!evaluation.Score.bOccluded)
		{
			return index;
		}
	}
	return INDEX_NONE;
}

/** Resets prior world feedback, then applies independent presentation states and Focus highlighting. */
void ULRInteractionComponent::ApplyPresentationStates(const TArray<FEvaluation>& evaluations, const int32 focusedIndex)
{
	for (const TWeakObjectPtr<ULRInteractionPresentationComponent>& component : PresentedComponents)
	{
		if (component.IsValid())
		{
			component->SetPresentationState(ELRInteractionPresentationState::None);
		}
	}
	PresentedComponents.Reset();
	TMap<TWeakObjectPtr<AActor>, ELRInteractionPresentationState> states;
	for (int32 index = 0; index < evaluations.Num(); ++index)
	{
		const FEvaluation& evaluation = evaluations[index];
		if (!evaluation.bShowHint || !evaluation.Actor.IsValid())
		{
			continue;
		}
		ELRInteractionPresentationState& state = states.FindOrAdd(evaluation.Actor);
		state = static_cast<ELRInteractionPresentationState>(FMath::Max(static_cast<uint8>(state),
			static_cast<uint8>(index == focusedIndex ? ELRInteractionPresentationState::Focused : evaluation.PresentationState)));
	}
	for (const TPair<TWeakObjectPtr<AActor>, ELRInteractionPresentationState>& pair : states)
	{
		if (AActor* actor = pair.Key.Get())
		{
			if (ULRInteractionPresentationComponent* component = actor->FindComponentByClass<ULRInteractionPresentationComponent>())
			{
				component->SetPresentationState(pair.Value);
				PresentedComponents.Add(component);
			}
		}
	}
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
void ULRInteractionComponent::ApplySelection(const TArray<FEvaluation>& evaluations, const int32 selectedIndex)
{
	AActor* selectedTarget = evaluations.IsValidIndex(selectedIndex) ? evaluations[selectedIndex].Actor.Get() : nullptr;
	const FLRInteractionOption selectedOption = selectedTarget ? evaluations[selectedIndex].Option : FLRInteractionOption();
	const ELRInteractionRange selectedRange = selectedTarget ? ELRInteractionRange::Executable : ELRInteractionRange::None;
	const bool bChanged = CurrentTarget.Get() != selectedTarget || CurrentOption.ActionTag != selectedOption.ActionTag
		|| CurrentRange != selectedRange;
	CurrentTarget = selectedTarget;
	CurrentOption = selectedOption;
	CurrentRange = selectedRange;
	if (bChanged)
	{
		OnTargetChanged.Broadcast(selectedTarget, CurrentOption, CurrentRange);
		CurrentPrompt.Target = selectedTarget;
		CurrentPrompt.Prompt = selectedOption.Prompt;
		CurrentPrompt.ActionTag = selectedOption.ActionTag;
		const APawn* ownerPawn = Cast<APawn>(GetOwner());
		const ALRPlayerController* controller = ownerPawn ? Cast<ALRPlayerController>(ownerPawn->GetController()) : nullptr;
		ULRInputConfig* inputConfig = controller ? controller->GetInputConfig() : nullptr;
		CurrentPrompt.InputAction = inputConfig ? inputConfig->InteractAction : nullptr;
		CurrentPrompt.bVisible = selectedTarget != nullptr;
		OnFocusedInteractionChanged.Broadcast(CurrentPrompt);
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
