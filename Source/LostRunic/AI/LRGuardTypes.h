/**
 * @file LRGuardTypes.h
 * @brief Guard 感知事实、0-11 警戒快照和 StateTree 行为契约。
 *
 * Alert 数值由 ULRAlertComponent 所有；Knowledge 只保存感知记忆；导航执行状态由
 * ALRGuardAIController 所有。这里不保存 Exposure、VisibilityScore 或 Search 行为状态。
 */
#pragma once

#include "CoreMinimal.h"
#include "Core/LRTypes.h"
#include "GameplayTagContainer.h"

#include "LRGuardTypes.generated.h"

class AActor;

/** Guard 的稳定行为枚举。Search 槽位保留给已序列化 UE 资产，玩法代码永不返回该值。 */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Guard Behavior"))
enum class ELRGuardBehaviorState : uint8
{
	IdlePatrol = 0 UMETA(DisplayName = "Idle / Patrol"),
	Suspicious = 1 UMETA(DisplayName = "Suspicious"),
	Investigate = 2 UMETA(DisplayName = "Investigate"),
	Search_DEPRECATED = 3 UMETA(Hidden),
	Chase = 4 UMETA(DisplayName = "Chase"),
	Stunned = 5 UMETA(DisplayName = "Stunned")
};

/** 0 隐藏、1-5 白色、6-10 红色、11 满红。 */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Guard Alert Tier"))
enum class ELRGuardAlertTier : uint8
{
	Hidden UMETA(DisplayName = "Hidden"),
	White UMETA(DisplayName = "White"),
	Red UMETA(DisplayName = "Red"),
	Full UMETA(DisplayName = "Full")
};

/** 区分 UE Hearing 和房间传播，避免把传播来源误当成普通 Hearing。 */
UENUM(BlueprintType, meta = (DisplayName = "Lost Runic Guard Noise Propagation"))
enum class ELRGuardNoisePropagationMode : uint8
{
	Hearing UMETA(DisplayName = "Hearing"),
	CurrentRoom UMETA(DisplayName = "Current Room"),
	AdjacentRoom UMETA(DisplayName = "Adjacent Room")
};

/** Alert 组件对外发布的只读 UI/行为快照；Fraction 是当前白/红档位内的 0-1 进度。 */
USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Alert Snapshot"))
struct LOSTRUNIC_API FLRAlertSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "警戒")
	int32 Level = 0;

	UPROPERTY(BlueprintReadOnly, Category = "警戒")
	float Fraction = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "警戒")
	ELRGuardAlertTier Tier = ELRGuardAlertTier::Hidden;

	/** 旧 UI 字段，实际行为由 AwarenessSnapshot.ResolvedBehavior 提供。 */
	UPROPERTY(BlueprintReadOnly, Category = "警戒", meta = (DeprecatedProperty))
	ELRGuardBehaviorState Behavior = ELRGuardBehaviorState::IdlePatrol;

	UPROPERTY(BlueprintReadOnly, Category = "警戒")
	bool bFullAlert = false;
};

/** Controller 维护的 Guard 感知记忆；只保存事实，不复制 UE Sight 的几何判定。 */
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

	/** Guard 当前真正要调查的唯一位置；历史事实仍分别保存在上面两个字段。 */
	UPROPERTY(BlueprintReadOnly, Category = "Guard|Knowledge")
	FVector LatestInvestigationLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Guard|Knowledge")
	bool bHasLatestInvestigationLocation = false;

	/** 经过 Raw Sight、Hard Hidden 和目标权限过滤后的有效视觉结果。 */
	UPROPERTY(BlueprintReadOnly, Category = "Guard|Knowledge")
	bool bCurrentlyVisible = false;

	UPROPERTY(BlueprintReadOnly, Category = "Guard|Knowledge")
	TWeakObjectPtr<AActor> LastAcceptedStimulusSource;

	UPROPERTY(BlueprintReadOnly, Category = "Guard|Knowledge")
	FGameplayTag LastAcceptedStimulusReason;

	UPROPERTY(BlueprintReadOnly, Category = "Guard|Knowledge", meta = (Units = "s"))
	float LastAcceptedStimulusTimeSeconds = 0.0f;
};

/** Controller 对 StateTree、UI 和调试输出提供的合并快照。 */
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

/** 已被 Controller 接受的噪声事件；SourcePace 在发声时快照，接收端不读取角色当前步态。 */
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

	/** 脚步发出瞬间的步态；门、机关等非脚步声音不使用 Pace 倍率。 */
	UPROPERTY(BlueprintReadOnly, Category = "Guard|Noise")
	ELRMovementPace SourcePace = ELRMovementPace::Walk;

	UPROPERTY(BlueprintReadOnly, Category = "Guard|Noise")
	bool bHasSourcePace = false;

	UPROPERTY(BlueprintReadOnly, Category = "Guard|Noise", meta = (Units = "s"))
	float TimeSeconds = 0.0f;
};
