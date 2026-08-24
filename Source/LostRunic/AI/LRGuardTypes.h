/**
 * @file LRGuardTypes.h
 * @brief 实现“家”垂直切片的守卫感知、0-11 警戒值、StateTree 行为切换、调查追逐与捕获死亡流程。规则层只计算状态，Controller 负责接入 UE 感知、导航和计时器。
 *
 * 关联文件：AI 目录内调用该公共契约的实现文件；所属领域：AI。
 * 设计依据：Docs/Technical/08_ArchitectureBoundaries.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "LRGuardTypes.generated.h"

class AActor;

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Guard Behavior"))
enum class ELRGuardBehaviorState : uint8
{
	IdlePatrol UMETA(DisplayName = "Idle / Patrol"),
	Suspicious UMETA(DisplayName = "Suspicious"),
	Investigate UMETA(DisplayName = "Investigate"),
	Search UMETA(DisplayName = "Search"),
	Chase UMETA(DisplayName = "Chase"),
	Stunned UMETA(DisplayName = "Stunned")
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Guard Alert Tier"))
enum class ELRGuardAlertTier : uint8
{
	Hidden UMETA(DisplayName = "Hidden"),
	White UMETA(DisplayName = "White"),
	Red UMETA(DisplayName = "Red"),
	Full UMETA(DisplayName = "Full")
};

/** Continuous sight exposure stage. Alert remains a separate controller-owned value. */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Guard Detection Stage"))
enum class ELRGuardDetectionStage : uint8
{
	None UMETA(DisplayName = "None"),
	Suspicious UMETA(DisplayName = "Suspicious"),
	Investigate UMETA(DisplayName = "Investigate"),
	Confirmed UMETA(DisplayName = "Confirmed")
};

/** Distinguishes direct hearing from room propagation without encoding it as a reason tag. */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Guard Noise Propagation"))
enum class ELRGuardNoisePropagationMode : uint8
{
	Hearing UMETA(DisplayName = "Hearing"),
	CurrentRoom UMETA(DisplayName = "Current Room"),
	AdjacentRoom UMETA(DisplayName = "Adjacent Room")
};

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Alert Snapshot"))
struct LOSTRUNIC_API FLRAlertSnapshot
{
	GENERATED_BODY()

	/** Level 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `0`。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Alert")
	int32 Level = 0;

	/** Fraction 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `0.0f`。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Alert")
	float Fraction = 0.0f;

	/** Tier 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `ELRGuardAlertTier::Hidden`。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Alert")
	ELRGuardAlertTier Tier = ELRGuardAlertTier::Hidden;

	/** Behavior 的领域数据，由所属类型负责维护和校验。 C++ 安全默认值为 `ELRGuardBehaviorState::IdlePatrol`。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Alert", meta = (DeprecatedProperty))
	ELRGuardBehaviorState Behavior = ELRGuardBehaviorState::IdlePatrol;

	/** Full Alert 的开关；true 表示启用，false 表示禁用。 C++ 安全默认值为 `false`。 蓝图可读取但不可写入。 */
	UPROPERTY(BlueprintReadOnly, Category = "Alert")
	bool bFullAlert = false;
};

/** Pure continuous sight sample. Gate failures always force VisibilityScore to zero. */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Guard Visibility Result"))
struct LOSTRUNIC_API FLRGuardVisibilityResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Guard|Visibility")
	bool bRangeGate = false;

	UPROPERTY(BlueprintReadOnly, Category = "Guard|Visibility")
	bool bConeGate = false;

	UPROPERTY(BlueprintReadOnly, Category = "Guard|Visibility")
	bool bLOSGate = false;

	UPROPERTY(BlueprintReadOnly, Category = "Guard|Visibility")
	bool bValidContactGate = false;

	UPROPERTY(BlueprintReadOnly, Category = "Guard|Visibility")
	bool bHardVisibilityGate = false;

	UPROPERTY(BlueprintReadOnly, Category = "Guard|Visibility")
	float DistanceFactor = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Guard|Visibility")
	float MovementFactor = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Guard|Visibility")
	float ExposureFactor = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Guard|Visibility")
	float LightingFactor = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Guard|Visibility")
	float PostureFactor = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Guard|Visibility")
	float VisibilityScore = 0.0f;

	/** Returns true only for a fully passing, positive-score sight sample. */
	bool IsActive() const
	{
		return bRangeGate && bConeGate && bLOSGate && bValidContactGate && bHardVisibilityGate
			&& VisibilityScore > 0.0f;
	}
};

/** Read-only knowledge accumulated by the guard perception/controller boundary. */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Guard Knowledge Snapshot"))
struct LOSTRUNIC_API FLRGuardKnowledgeSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Guard|Knowledge")
	TWeakObjectPtr<AActor> VisualCandidate;

	UPROPERTY(BlueprintReadOnly, Category = "Guard|Knowledge")
	bool bHasVisualCandidate = false;

	UPROPERTY(BlueprintReadOnly, Category = "Guard|Knowledge")
	TWeakObjectPtr<AActor> ConfirmedThreat;

	UPROPERTY(BlueprintReadOnly, Category = "Guard|Knowledge")
	bool bHasConfirmedThreat = false;

	UPROPERTY(BlueprintReadOnly, Category = "Guard|Knowledge")
	FVector LastKnownThreatLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Guard|Knowledge")
	bool bHasLastKnownThreatLocation = false;

	UPROPERTY(BlueprintReadOnly, Category = "Guard|Knowledge")
	FVector LastDisturbanceLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Guard|Knowledge")
	bool bHasLastDisturbanceLocation = false;

	UPROPERTY(BlueprintReadOnly, Category = "Guard|Knowledge")
	FLRGuardVisibilityResult CurrentVisibility;

	UPROPERTY(BlueprintReadOnly, Category = "Guard|Knowledge")
	bool bPendingThreatInvestigation = false;

	UPROPERTY(BlueprintReadOnly, Category = "Guard|Knowledge", meta = (ClampMin = "0.0", Units = "s"))
	float EffectiveExposureSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Guard|Knowledge")
	ELRGuardDetectionStage Stage = ELRGuardDetectionStage::None;

	UPROPERTY(BlueprintReadOnly, Category = "Guard|Knowledge")
	TWeakObjectPtr<AActor> LastAcceptedStimulusSource;

	UPROPERTY(BlueprintReadOnly, Category = "Guard|Knowledge")
	FGameplayTag LastAcceptedStimulusReason;

	UPROPERTY(BlueprintReadOnly, Category = "Guard|Knowledge", meta = (Units = "s"))
	float LastAcceptedStimulusTimeSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Guard|Knowledge", meta = (ClampMin = "0"))
	int32 InvestigationContextRevision = 0;
};

/** Combined controller-facing awareness view; Alert remains independently owned by ULRAlertComponent. */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Guard Awareness Snapshot"))
struct LOSTRUNIC_API FLRGuardAwarenessSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Guard|Awareness")
	FLRAlertSnapshot Alert;

	UPROPERTY(BlueprintReadOnly, Category = "Guard|Awareness")
	FLRGuardKnowledgeSnapshot Knowledge;

	UPROPERTY(BlueprintReadOnly, Category = "Guard|Awareness")
	ELRGuardBehaviorState ResolvedBehavior = ELRGuardBehaviorState::IdlePatrol;

	UPROPERTY(BlueprintReadOnly, Category = "Guard|Awareness")
	FVector InvestigationLocation = FVector::ZeroVector;
};

/** Accepted noise information; propagation mode prevents room events being mistaken for hearing events. */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Guard Noise Stimulus"))
struct LOSTRUNIC_API FLRGuardNoiseStimulus
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Guard|Noise")
	TWeakObjectPtr<AActor> Source;

	UPROPERTY(BlueprintReadOnly, Category = "Guard|Noise")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Guard|Noise")
	FGameplayTag Reason;

	UPROPERTY(BlueprintReadOnly, Category = "Guard|Noise")
	ELRGuardNoisePropagationMode PropagationMode = ELRGuardNoisePropagationMode::Hearing;

	UPROPERTY(BlueprintReadOnly, Category = "Guard|Noise", meta = (Units = "s"))
	float TimeSeconds = 0.0f;
};
