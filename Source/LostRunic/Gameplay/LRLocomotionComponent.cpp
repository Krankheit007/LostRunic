/**
 * @file LRLocomotionComponent.cpp
 * @brief 根据心理状态和玩家切换请求选择潜行/走路/奔跑，以 80/150/250 cm/s 基线移动，并按移动距离和环境发布脚步噪声。
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
#include "Framework/LRGameInstanceSubsystem.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
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
	if (!ensureMsgf(Character && Tuning, TEXT("%s requires an ACharacter owner and Movement tuning."), *GetNameSafe(this)))
	{
		return;
	}

	LastSampleLocation = Character->GetActorLocation();
	SetPace(Pace);
	GetWorld()->GetTimerManager().SetTimer(SampleTimer, this, &ULRLocomotionComponent::SampleTravelDistance,
		Tuning->SampleIntervalSeconds, true);
}

/**
 * @brief 解除委托并清理计时器或缓存，避免关卡切换和对象销毁后继续收到回调。
 * @param endPlayReason Unreal 提供的结束原因，用于区分销毁、关卡切换和退出。
 */
void ULRLocomotionComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(SampleTimer);
	}
	Super::EndPlay(endPlayReason);
}

/**
 * @brief 更新 Pace，并在需要时同步组件状态或广播变化事件。
 * @param newPace 本次操作使用的 `newPace` 枚举或模式值。
 */
void ULRLocomotionComponent::SetPace(const ELRMovementPace newPace)
{
	Pace = newPace;
	if (!Character || !Tuning)
	{
		return;
	}

	float speed = Tuning->WalkSpeed;
	if (Pace == ELRMovementPace::Sneak)
	{
		speed = Tuning->SneakSpeed;
	}
	else if (Pace == ELRMovementPace::Run)
	{
		speed = Tuning->RunSpeed;
	}
	Character->GetCharacterMovement()->MaxWalkSpeed = speed;
}

/**
 * @brief 在状态允许时切换潜行与走路；Perception 强制潜行，Courage 禁止潜行。
 */
void ULRLocomotionComponent::ToggleSneak()
{
	ELRMovementPace& targetPace = Pace == ELRMovementPace::Run ? PaceBeforeRun : Pace;
	targetPace = targetPace == ELRMovementPace::Sneak ? ELRMovementPace::Walk : ELRMovementPace::Sneak;
	if (Pace != ELRMovementPace::Run)
	{
		SetPace(targetPace);
	}
}

/**
 * @brief 开始 Start Run 流程，建立本次操作拥有的状态、委托或计时器。
 */
void ULRLocomotionComponent::StartRun()
{
	if (Pace == ELRMovementPace::Run)
	{
		return;
	}

	PaceBeforeRun = Pace;
	SetPace(ELRMovementPace::Run);
}

/**
 * @brief 结束或取消 Stop Run 流程，并清理本次操作拥有的临时状态。
 */
void ULRLocomotionComponent::StopRun()
{
	if (Pace == ELRMovementPace::Run)
	{
		SetPace(PaceBeforeRun);
	}
}

/**
 * @brief 以低频计时器累计角色实际位移，达到步长后发布脚步而不使用 Tick。
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
	const FGameplayTag noiseTag = Pace == ELRMovementPace::Run ? LRGameplayTags::NoiseFootstepRun : LRGameplayTags::NoiseFootstepWalk;
	OnFootstep.Broadcast(location, GetNoiseRadius(), noiseTag);
}

/**
 * @brief 查询 Step Distance；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
float ULRLocomotionComponent::GetStepDistance() const
{
	return Pace == ELRMovementPace::Run ? Tuning->RunStepDistance : Tuning->WalkStepDistance;
}

/**
 * @brief 查询 Noise Radius；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
float ULRLocomotionComponent::GetNoiseRadius() const
{
	if (Pace == ELRMovementPace::Sneak)
	{
		return NoiseEnvironment == ELRNoiseEnvironment::Outdoor ? Tuning->OutdoorSneakGuardNoiseRadius : 0.0f;
	}
	if (Pace == ELRMovementPace::Run)
	{
		return Tuning->IndoorRunNoiseRadius;
	}
	return NoiseEnvironment == ELRNoiseEnvironment::Outdoor ? Tuning->OutdoorAlertGuardNoiseRadius : Tuning->IndoorWalkNoiseRadius;
}
