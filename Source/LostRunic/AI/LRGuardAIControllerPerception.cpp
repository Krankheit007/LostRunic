/**
 * @file LRGuardAIControllerPerception.cpp
 * @brief 守卫控制器感知实现：Sight/Hearing 刺激转换为警戒语义入口、感知配置、遮挡与隐藏判定、捕获计时（眩晕期间跳过）。
 *
 * 关联文件：LRGuardAIController.h；所属领域：AI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "AI/LRGuardAIController.h"

#include "AI/LRAlertComponent.h"
#include "AI/LRGuardCharacter.h"
#include "AI/LRGuardPerceptionRules.h"
#include "Core/LRGameplayTags.h"
#include "Data/LRGuardTuning.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Stealth/LRGuardVisibility.h"

/**
 * @brief 把 UE 感知刺激转换为可见/听见事件、异常位置和警戒原因标签；听觉走 ResolveNoiseAlertDelta 语义（吸引/Set 分派）。
 * @param actor 本次查询、交互或事件涉及的 Actor。
 * @param stimulus 时间值 `stimulus`，单位为秒。
 */
void ALRGuardAIController::HandlePerception(AActor* actor, const FAIStimulus stimulus)
{
	if (!actor || !Alert.IsValid())
	{
		return;
	}
	if (stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
	{
		const bool bVisible = stimulus.WasSuccessfullySensed() && CanConfirmSight(actor);
		Alert->SetSightTarget(actor, bVisible, stimulus.StimulusLocation);
	}
	else if (stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>() && stimulus.WasSuccessfullySensed())
	{
		FGameplayTag reason = FGameplayTag::RequestGameplayTag(stimulus.Tag, false);
		if (!reason.IsValid())
		{
			reason = LRGameplayTags::NoiseInteraction;
		}
		const FLRNoiseResponse response = LRGuardPerceptionRules::ResolveNoiseAlertDelta(
			reason, Alert->GetAlertLevel(), GetEffectiveTuning());
		if (!response.bRespond)
		{
			return;
		}
		if (response.bIsAttract)
		{
			Alert->ApplyAttract(stimulus.StimulusLocation, actor, reason);
		}
		else
		{
			Alert->ApplyAlertDelta(response.Delta, stimulus.StimulusLocation, actor, reason);
		}
	}
}

/**
 * @brief 用 Guard 调优资产配置 UE Sight/Hearing 感知，包括完整视野角换算、距离和阵营检测。
 */
void ALRGuardAIController::ConfigurePerception()
{
	const ULRGuardTuning& tuning = GetEffectiveTuning();
	SightConfig->SightRadius = tuning.SightRadius;
	SightConfig->LoseSightRadius = tuning.LoseSightRadius;
	SightConfig->PeripheralVisionAngleDegrees = tuning.SightConeDegrees * 0.5f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	HearingConfig->HearingRange = tuning.MaxHearingRange * tuning.HearingRangeMultiplier;
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	AIPerception->ConfigureSense(*SightConfig);
	AIPerception->ConfigureSense(*HearingConfig);
	AIPerception->SetDominantSense(SightConfig->GetSenseImplementation());
}

/**
 * @brief 按可调低频计时检查追逐目标距离；进入捕获半径后触发玩家死亡与 Memory 流程；眩晕期间跳过。
 */
void ALRGuardAIController::HandleCaptureTimer()
{
	if (bStunned || !Alert.IsValid() || Alert->GetBehaviorState() != ELRGuardBehaviorState::Chase)
	{
		return;
	}
	AActor* target = Alert->GetTargetActor();
	if (!CanConfirmSight(target))
	{
		Alert->SetSightTarget(target, false, target ? target->GetActorLocation() : FVector::ZeroVector);
		return;
	}
	ALRGuardCharacter* guard = Cast<ALRGuardCharacter>(GetPawn());
	if (guard && FVector::Dist2D(guard->GetActorLocation(), target->GetActorLocation()) <= GetEffectiveTuning().CaptureRadius)
	{
		guard->CaptureTarget(target);
	}
}

/**
 * @brief 判断 Can Confirm Sight 对应条件；不产生玩法副作用。
 * @param actor 本次查询、交互或事件涉及的 Actor。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ALRGuardAIController::CanConfirmSight(AActor* actor) const
{
	const APawn* guardPawn = GetPawn();
	if (!actor || !guardPawn)
	{
		return false;
	}
	const FVector toTarget = actor->GetActorLocation() - guardPawn->GetActorLocation();
	const float distance = toTarget.Size2D();
	const float forwardDot = FVector::DotProduct(guardPawn->GetActorForwardVector().GetSafeNormal2D(),
		toTarget.GetSafeNormal2D());
	return LRGuardPerceptionRules::CanConfirmSight(distance, forwardDot, !LineOfSightTo(actor),
		IsHiddenFromGuard(actor), GetEffectiveTuning());
}

/**
 * @brief 判断 Is Hidden From Guard 对应条件；不产生玩法副作用。
 * @param actor 本次查询、交互或事件涉及的 Actor。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ALRGuardAIController::IsHiddenFromGuard(AActor* actor) const
{
	if (actor->GetClass()->ImplementsInterface(ULRGuardVisibility::StaticClass()))
	{
		return !ILRGuardVisibility::Execute_IsVisibleToGuard(actor, const_cast<ALRGuardAIController*>(this));
	}
	for (UActorComponent* component : actor->GetComponents())
	{
		if (component && component->GetClass()->ImplementsInterface(ULRGuardVisibility::StaticClass())
			&& !ILRGuardVisibility::Execute_IsVisibleToGuard(component, const_cast<ALRGuardAIController*>(this)))
		{
			return true;
		}
	}
	return false;
}
