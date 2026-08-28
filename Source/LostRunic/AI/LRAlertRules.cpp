/**
 * @file LRAlertRules.cpp
 * @brief Guard 警戒纯规则实现。
 */
#include "AI/LRAlertRules.h"

#include "Data/LRGuardTuning.h"

namespace
{
	float ResolvePaceMultiplier(const ELRMovementPace sourcePace, const FLRGuardTuningSettings& tuning)
	{
		switch (sourcePace)
		{
		case ELRMovementPace::Run:
			return tuning.FirstAttractRunCooldownMultiplier;
		case ELRMovementPace::Sneak:
			return tuning.FirstAttractSneakCooldownMultiplier;
		case ELRMovementPace::Walk:
		default:
			return tuning.FirstAttractWalkCooldownMultiplier;
		}
	}
}

int32 LRAlertRules::ApplyDelta(const int32 currentLevel, const int32 delta)
{
	return FMath::Clamp(currentLevel + delta, MinAlertLevel, MaxAlertLevel);
}

ELRGuardBehaviorState LRAlertRules::ResolveState(const int32 alertLevel)
{
	const int32 clampedLevel = FMath::Clamp(alertLevel, MinAlertLevel, MaxAlertLevel);
	if (clampedLevel == MinAlertLevel)
	{
		return ELRGuardBehaviorState::IdlePatrol;
	}
	if (clampedLevel <= SuspiciousMaxLevel)
	{
		return ELRGuardBehaviorState::Suspicious;
	}
	// Alert 11 is only a numeric band. Knowledge and current effective sight
	// are required by ResolveTargetBehavior before Chase can be selected.
	return ELRGuardBehaviorState::Investigate;
}

bool LRAlertRules::IsChaseEligible(const FLRAlertSnapshot& alert,
	const FLRGuardKnowledgeSnapshot& knowledge)
{
	return alert.Level >= ConfirmedAlertLevel
		&& knowledge.bHasConfirmedThreat
		&& knowledge.ConfirmedThreat.IsValid()
		&& knowledge.bHasVisualCandidate
		&& knowledge.VisualCandidate.IsValid()
		&& knowledge.VisualCandidate.Get() == knowledge.ConfirmedThreat.Get()
		&& knowledge.bCurrentlyVisible;
}

ELRGuardBehaviorState LRAlertRules::ResolveTargetBehavior(const bool bStunned,
	const FLRAlertSnapshot& alert, const FLRGuardKnowledgeSnapshot& knowledge)
{
	if (bStunned)
	{
		return ELRGuardBehaviorState::Stunned;
	}
	return IsChaseEligible(alert, knowledge) ? ELRGuardBehaviorState::Chase
		: ResolveState(alert.Level);
}

ELRGuardAlertTier LRAlertRules::ResolveAlertTier(const int32 alertLevel)
{
	if (alertLevel <= MinAlertLevel)
	{
		return ELRGuardAlertTier::Hidden;
	}
	if (alertLevel <= SuspiciousMaxLevel)
	{
		return ELRGuardAlertTier::White;
	}
	if (alertLevel <= InvestigateMaxLevel)
	{
		return ELRGuardAlertTier::Red;
	}
	return ELRGuardAlertTier::Full;
}

FVector LRAlertRules::ResolveInvestigationLocation(const FLRGuardKnowledgeSnapshot& knowledge)
{
	return knowledge.bHasLatestInvestigationLocation
		? knowledge.LatestInvestigationLocation : FVector::ZeroVector;
}

float LRAlertRules::ResolveAttractCooldown(const int32 resultAlertLevel, const bool bFirstAttractInBand,
	const ELRMovementPace sourcePace, const bool bUsePaceMultiplier,
	const FLRGuardTuningSettings& tuning)
{
	const float baseCooldown = resultAlertLevel <= SuspiciousMaxLevel
		? tuning.SuspiciousStimulusCooldownSeconds : tuning.InvestigateStimulusCooldownSeconds;
	return bFirstAttractInBand && bUsePaceMultiplier
		? baseCooldown * ResolvePaceMultiplier(sourcePace, tuning) : baseCooldown;
}

bool LRAlertRules::IsIncreaseAllowed(const double nowSeconds, const double lastIncreaseTimeSeconds,
	const float cooldownSeconds)
{
	return cooldownSeconds <= 0.0f || nowSeconds - lastIncreaseTimeSeconds >= cooldownSeconds;
}
