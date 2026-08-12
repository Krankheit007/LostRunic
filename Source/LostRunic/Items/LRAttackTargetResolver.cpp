/**
 * @file LRAttackTargetResolver.cpp
 * @brief 独立攻击目标筛选：只保留实现 ILRAttackTarget、满足攻击距离、朝向、遮挡和目标状态（免疫）的候选，不复用交互系统的当前焦点、目标接口或 HUD 提示语义。
 *
 * 关联文件：LRAttackTargetResolver.h；所属领域：Items。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Items/LRAttackTargetResolver.h"

#include "Core/LRGameplayTags.h"
#include "Data/LRGameTuningSet.h"
#include "Data/LRStateTuning.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "GameFramework/Actor.h"
#include "Items/LRAttackTarget.h"
#include "CollisionQueryParams.h"
#include "Kismet/GameplayStatics.h"

/**
 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
 */
ULRAttackTargetResolver::ULRAttackTargetResolver()
{
	PrimaryComponentTick.bCanEverTick = false;
}

/**
 * @brief 在合法攻击候选中选择最近的未免疫目标；全部失败时返回 INDEX_NONE。
 * @param candidates 本次领域操作的结构化数据 `candidates`；字段语义由对应 USTRUCT 定义。
 * @param tuning 数据或调优来源 `tuning`；调用期间只读，并按稳定 ID 解析内容。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
int32 LRAttackTargetRules::SelectBestTarget(const TArray<FLRAttackCandidateScore>& candidates,
	const ULRStateTuning& tuning)
{
	int32 bestIndex = INDEX_NONE;
	float bestDistanceSquared = MAX_FLT;
	for (int32 index = 0; index < candidates.Num(); ++index)
	{
		const FLRAttackCandidateScore& candidate = candidates[index];
		if (!candidate.bInRange || !candidate.bFacingAllowed || !candidate.bNotOccluded || !candidate.bVulnerable)
		{
			continue;
		}
		if (candidate.DistanceSquared < bestDistanceSquared)
		{
			bestDistanceSquared = candidate.DistanceSquared;
			bestIndex = index;
		}
	}
	return bestIndex;
}

/**
 * @brief 判断 Is Facing Allowed 对应条件；不产生玩法副作用。
 * @param forwardDot 调用方提供的 `forwardDot`，只在本次操作范围内使用。
 * @param tuning 数据或调优来源 `tuning`；调用期间只读，并按稳定 ID 解析内容。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool LRAttackTargetRules::IsFacingAllowed(const float forwardDot, const ULRStateTuning& tuning)
{
	const float halfAngleRadians = FMath::DegreesToRadians(tuning.CourageAttackFacingDegrees * 0.5f);
	return forwardDot >= FMath::Cos(halfAngleRadians);
}

/**
 * @brief 判断 Is In Range 对应条件；不产生玩法副作用。
 * @param distanceSquared 空间值 `distanceSquared`；距离和位置使用 Unreal 厘米单位。
 * @param tuning 数据或调优来源 `tuning`；调用期间只读，并按稳定 ID 解析内容。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool LRAttackTargetRules::IsInRange(const float distanceSquared, const ULRStateTuning& tuning)
{
	return distanceSquared <= FMath::Square(tuning.CourageAttackRangeCm);
}

/**
 * @brief 扫描世界并返回唯一最近的合法攻击目标；未找到时返回 false 并给出原因标签。
 * @param instigator 攻击发起者；用于距离、朝向和遮挡计算。
 * @param outTarget 本次领域操作的结构化数据 `outTarget`；字段语义由对应 USTRUCT 定义。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRAttackTargetResolver::FindAttackTarget(AActor* instigator, AActor*& outTarget) const
{
	outTarget = nullptr;
	if (!instigator || !GetWorld())
	{
		return false;
	}
	const ULRStateTuning& tuning = GetEffectiveTuning();

	TArray<AActor*> candidates;
	UGameplayStatics::GetAllActorsWithInterface(this, ULRAttackTarget::StaticClass(), candidates);

	TArray<FLRAttackCandidateScore> scores;
	const FVector origin = instigator->GetActorLocation();
	const FVector forward = instigator->GetActorForwardVector().GetSafeNormal2D();
	for (AActor* candidate : candidates)
	{
		if (!candidate || candidate == instigator)
		{
			continue;
		}
		FLRAttackCandidateScore& score = scores.AddDefaulted_GetRef();
		const FVector toTarget = candidate->GetActorLocation() - origin;
		const FVector toTarget2D = toTarget.GetSafeNormal2D();
		score.DistanceSquared = toTarget.SizeSquared2D();
		score.FacingDot = FVector::DotProduct(forward, toTarget2D);
		score.bInRange = LRAttackTargetRules::IsInRange(score.DistanceSquared, tuning);
		score.bFacingAllowed = LRAttackTargetRules::IsFacingAllowed(score.FacingDot, tuning);
		score.bNotOccluded = !IsOccluded(instigator, candidate);
		const FGameplayTagContainer targetTags = ILRAttackTarget::Execute_GetAttackTargetTags(candidate);
		score.bVulnerable = !targetTags.HasTag(LRGameplayTags::TargetGuardCourageImmune);
	}

	const int32 bestIndex = LRAttackTargetRules::SelectBestTarget(scores, tuning);
	if (bestIndex == INDEX_NONE)
	{
		return false;
	}
	outTarget = candidates[bestIndex];
	return true;
}

/**
 * @brief 判断 Is Occluded 对应条件；不产生玩法副作用。
 * @param instigator 攻击发起者；用于距离、朝向和遮挡计算。
 * @param target 本次规则检查或操作的目标对象。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRAttackTargetResolver::IsOccluded(AActor* instigator, const AActor* target) const
{
	if (!instigator || !target)
	{
		return true;
	}
	FCollisionQueryParams queryParams(TEXT("LRAttackTargetOcclusion"), true, instigator);
	FHitResult hit;
	const FVector start = instigator->GetActorLocation() + FVector::UpVector * 100.0f;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(hit, start, target->GetActorLocation(), ECC_Visibility, queryParams);
	if (!bHit)
	{
		return false;
	}
	const AActor* hitActor = hit.GetActor();
	return hitActor != target && !hitActor->IsOwnedBy(target) && !target->IsOwnedBy(hitActor);
}

/**
 * @brief 查询 Effective Tuning；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
const ULRStateTuning& ULRAttackTargetResolver::GetEffectiveTuning() const
{
	if (Tuning)
	{
		return *Tuning;
	}
	const UGameInstance* gameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const ULRGameInstanceSubsystem* subsystem = gameInstance ? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	const ULRStateTuning* resolved = subsystem && subsystem->GetTuningSet()
		? subsystem->GetTuningSet()->State : GetDefault<ULRStateTuning>();
	if (resolved)
	{
		const_cast<ULRAttackTargetResolver*>(this)->Tuning = resolved;
	}
	return resolved ? *resolved : *GetDefault<ULRStateTuning>();
}
