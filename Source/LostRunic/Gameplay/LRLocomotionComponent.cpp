/**
 * @file LRLocomotionComponent.cpp
 * @brief 根据心理状态和玩家切换请求选择潜行/走路/奔跑，以 80/150/250 cm/s 基线移动，并按移动距离和环境发布脚步噪声。玩家请求经状态步态规则验证，组件内部应用与掩体覆盖走独立通道。
 *
 * 关联文件：LRLocomotionComponent.h；所属领域：Gameplay。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Gameplay/LRLocomotionComponent.h"

#include "Core/LRGameplayTags.h"
#include "Core/LRLog.h"
#include "Data/LRGameTuningSet.h"
#include "Data/LRMovementTuning.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/LRCharacter.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Gameplay/LRMovementRules.h"
#include "State/LRStateComponent.h"
#include "TimerManager.h"

/**
 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
 */
ULRLocomotionComponent::ULRLocomotionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

/**
 * @brief 在进入世界后解析运行时依赖、绑定事件并启动所需计时器；构造阶段不访问 World 或玩家对象。
 */
void ULRLocomotionComponent::BeginPlay()
{
	Super::BeginPlay();
	Character = Cast<ACharacter>(GetOwner());
	const UGameInstance* gameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const ULRGameInstanceSubsystem* subsystem = gameInstance ? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	Tuning = subsystem && subsystem->GetTuningSet() ? subsystem->GetTuningSet()->Movement : nullptr;
	State = Cast<ALRCharacter>(GetOwner()) ? Cast<ALRCharacter>(GetOwner())->GetStateComponent() : nullptr;
	if (!ensureMsgf(Character && Tuning, TEXT("%s requires an ACharacter owner and Movement tuning."), *GetNameSafe(this)))
	{
		return;
	}

	LastSampleLocation = Character->GetActorLocation();
	ApplyPace(Pace, FGameplayTag());
	if (State.IsValid())
	{
		State->OnStateChanged.AddDynamic(this, &ULRLocomotionComponent::HandleStateChanged);
	}
	GetWorld()->GetTimerManager().SetTimer(SampleTimer, this, &ULRLocomotionComponent::SampleTravelDistance,
		Tuning->SampleIntervalSeconds, true);
}

/**
 * @brief 解除委托并清理计时器或缓存，避免关卡切换和对象销毁后继续收到回调。
 * @param endPlayReason Unreal 提供的结束原因，用于区分销毁、关卡切换和退出。
 */
void ULRLocomotionComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	if (State.IsValid())
	{
		State->OnStateChanged.RemoveDynamic(this, &ULRLocomotionComponent::HandleStateChanged);
	}
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(SampleTimer);
	}
	Super::EndPlay(endPlayReason);
}

/**
 * @brief 请求切换潜行与走路；受当前状态步态规则验证，Perception 强制潜行，Courage 禁止潜行，Memory 仅走路。
 */
void ULRLocomotionComponent::RequestToggleSneak()
{
	ELRMovementPace& targetPace = Pace == ELRMovementPace::Run ? PaceBeforeRun : Pace;
	const ELRMovementPace candidate = targetPace == ELRMovementPace::Sneak ? ELRMovementPace::Walk : ELRMovementPace::Sneak;
	if (!LRMovementRules::IsPaceAllowed(GetEffectiveMode(), candidate))
	{
		RejectPaceRequest(candidate);
		return;
	}
	if (Pace != ELRMovementPace::Run)
	{
		ApplyPace(candidate, FGameplayTag());
	}
	else
	{
		targetPace = candidate;
	}
}

/**
 * @brief 请求开始奔跑；当前状态禁止奔跑时拒绝并广播拒绝原因。
 */
void ULRLocomotionComponent::RequestStartRun()
{
	if (Pace == ELRMovementPace::Run)
	{
		return;
	}
	if (!LRMovementRules::IsPaceAllowed(GetEffectiveMode(), ELRMovementPace::Run))
	{
		RejectPaceRequest(ELRMovementPace::Run);
		return;
	}

	PaceBeforeRun = Pace;
	ApplyPace(ELRMovementPace::Run, FGameplayTag());
}

/**
 * @brief 请求结束奔跑，恢复到奔跑前的步态。
 */
void ULRLocomotionComponent::RequestStopRun()
{
	if (Pace == ELRMovementPace::Run)
	{
		ApplyPace(PaceBeforeRun, FGameplayTag());
	}
}

/**
 * @brief 组件内部应用步态（状态同步、掩体、调试）；玩家输入请走 Request* 入口。
 * @param newPace 本次操作使用的 `newPace` 枚举或模式值。
 * @param source 来源 Gameplay Tag，用于日志与诊断；None 表示常规状态应用。
 */
void ULRLocomotionComponent::ApplyPace(const ELRMovementPace newPace, const FGameplayTag source)
{
	if (Pace != newPace)
	{
		UE_LOG(LogLostRunicState, Verbose, TEXT("Locomotion=%s pace %d -> %d source=%s"), *GetNameSafe(this),
			static_cast<int32>(Pace), static_cast<int32>(newPace), *source.ToString());
	}
	Pace = newPace;
	SyncMovementSpeed();
}

/**
 * @brief 带来源标识的临时步态覆盖（如掩体强制潜行）；清除时按当前状态重新求值合法步态。
 * @param newPace 本次操作使用的 `newPace` 枚举或模式值。
 * @param source 来源 Gameplay Tag，用于标识覆盖的持有者。
 */
void ULRLocomotionComponent::OverridePace(const ELRMovementPace newPace, const FGameplayTag source)
{
	PaceOverride = newPace;
	PaceOverrideSource = source;
	SyncMovementSpeed();
}

/**
 * @brief 清除指定来源的临时步态覆盖；覆盖期间的基础步态可能因状态规则过期，清除后重新求值。
 * @param source 来源 Gameplay Tag，用于标识覆盖的持有者。
 */
void ULRLocomotionComponent::ClearPaceOverride(const FGameplayTag source)
{
	if (!PaceOverrideSource.IsValid() || PaceOverrideSource != source)
	{
		return;
	}
	PaceOverrideSource = FGameplayTag();
	if (!LRMovementRules::IsPaceAllowed(GetEffectiveMode(), Pace))
	{
		Pace = LRMovementRules::GetDefaultPace(GetEffectiveMode());
	}
	SyncMovementSpeed();
}

/**
 * @brief 处理 Handle State Changed 事件，将引擎回调转换为对应领域状态更新；清空掩体覆盖并按状态默认步态应用。
 * @param currentMode 本次操作使用的 `currentMode` 枚举或模式值。
 * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
 */
void ULRLocomotionComponent::HandleStateChanged(const ELRPerceptionMode currentMode, const FGameplayTag reason)
{
	// 掩体强制潜行不变量：状态变化不清除临时步态覆盖（如 Movement.Override.Hidden）；
	// 基础步态仍按状态默认更新，退出掩体时 ClearPaceOverride 会按当前状态重新求值。
	ApplyPace(LRMovementRules::GetDefaultPace(currentMode), reason);
}

/**
 * @brief 查询 Effective Mode；State 组件缺失时回退 Normal（无 World 测试场景）。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
ELRPerceptionMode ULRLocomotionComponent::GetEffectiveMode() const
{
	return State.IsValid() ? State->GetCurrentMode() : ELRPerceptionMode::Normal;
}

/**
 * @brief 查询 Effective Pace；掩体等覆盖存在时返回覆盖值，否则返回基础步态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
ELRMovementPace ULRLocomotionComponent::GetEffectivePace() const
{
	return PaceOverrideSource.IsValid() ? PaceOverride : Pace;
}

/**
 * @brief 以低频计时器累计角色实际位移，达到步长后按步态×环境解析并发布脚步而不使用 Tick。
 */
void ULRLocomotionComponent::SampleTravelDistance()
{
	if (!Character || !Tuning)
	{
		return;
	}

	const FVector location = Character->GetActorLocation();
	DistanceSinceFootstep += FVector::Dist2D(location, LastSampleLocation);
	LastSampleLocation = location;
	const float stepDistance = GetStepDistance();
	if (DistanceSinceFootstep < stepDistance || Character->GetVelocity().SizeSquared2D() <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	DistanceSinceFootstep = FMath::Fmod(DistanceSinceFootstep, stepDistance);
	const FLRNoiseResolution resolution = LRMovementRules::ResolveFootstepNoise(GetEffectivePace(), NoiseEnvironment, *Tuning);
	OnFootstep.Broadcast(location, resolution.Radius, resolution.Tag);
}

/**
 * @brief 查询 Step Distance；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
float ULRLocomotionComponent::GetStepDistance() const
{
	return GetEffectivePace() == ELRMovementPace::Run ? Tuning->RunStepDistance : Tuning->WalkStepDistance;
}

/**
 * @brief 按有效步态同步 CharacterMovement 的 MaxWalkSpeed。
 */
void ULRLocomotionComponent::SyncMovementSpeed()
{
	if (!Character || !Tuning)
	{
		return;
	}

	float speed = Tuning->WalkSpeed;
	const ELRMovementPace effectivePace = GetEffectivePace();
	if (effectivePace == ELRMovementPace::Sneak)
	{
		speed = Tuning->SneakSpeed;
	}
	else if (effectivePace == ELRMovementPace::Run)
	{
		speed = Tuning->RunSpeed;
	}
	Character->GetCharacterMovement()->MaxWalkSpeed = speed;
}

/**
 * @brief 对禁止的步态请求统一记录日志并广播拒绝事件。
 * @param requestedPace 本次操作使用的 `requestedPace` 枚举或模式值。
 */
void ULRLocomotionComponent::RejectPaceRequest(const ELRMovementPace requestedPace)
{
	UE_LOG(LogLostRunicState, Warning, TEXT("Locomotion=%s pace request %d rejected in mode %d reason=%s"),
		*GetNameSafe(this), static_cast<int32>(requestedPace), static_cast<int32>(GetEffectiveMode()),
		*LRGameplayTags::MovementRejectPaceForbidden.GetTag().ToString());
	OnPaceRequestRejected.Broadcast(requestedPace, LRGameplayTags::MovementRejectPaceForbidden);
}
