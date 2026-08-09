/**
 * @file LRStateComponent.cpp
 * @brief 维护玩家当前心理状态、眼部输入先按者优先门控、0.8 秒进入/0.3 秒返回长按和表现安全锁；关联 LRStateRules、LRStateTuning、LRStatePresentationComponent 与 LRPlayerController。
 *
 * 关联文件：LRStateComponent.h；所属领域：State。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "State/LRStateComponent.h"

#include "Core/LRGameplayTags.h"
#include "Core/LRLog.h"
#include "Data/LRGameTuningSet.h"
#include "Data/LRStateTuning.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "TimerManager.h"

/**
 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
 */
ULRStateComponent::ULRStateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

/**
 * @brief 在进入世界后解析运行时依赖、绑定事件并启动所需计时器；构造阶段不访问 World 或玩家对象。
 */
void ULRStateComponent::BeginPlay()
{
	Super::BeginPlay();
	const UGameInstance* gameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const ULRGameInstanceSubsystem* subsystem = gameInstance ? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	Tuning = subsystem && subsystem->GetTuningSet() ? subsystem->GetTuningSet()->State : nullptr;
	ensureMsgf(Tuning, TEXT("%s is using fallback State tuning because the project tuning set is unavailable."), *GetNameSafe(this));
}

/**
 * @brief 解除委托并清理计时器或缓存，避免关卡切换和对象销毁后继续收到回调。
 * @param endPlayReason Unreal 提供的结束原因，用于区分销毁、关卡切换和退出。
 */
void ULRStateComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
	}
	Super::EndPlay(endPlayReason);
}

/**
 * @brief 校验并提交四状态切换请求；成功时广播切换前后事件并启动表现锁，失败时返回原因标签。
 * @param request 不可变领域请求，包含本次操作所需的稳定 ID、来源、目标或原因。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FLRStateChangeResult ULRStateComponent::RequestStateChange(const FLRStateChangeRequest& request)
{
	FLRStateChangeResult result;
	result.PreviousMode = CurrentMode;
	result.CurrentMode = CurrentMode;
	const bool bDeathRequest = request.RequestType == ELRStateRequestType::Death;

	FGameplayTag rejectionReason;
	if (request.TargetMode == CurrentMode)
	{
		rejectionReason = LRGameplayTags::StateRejectAlreadyCurrent;
	}
	else if (bPresentationLocked && !bDeathRequest)
	{
		rejectionReason = LRGameplayTags::StateRejectPresentationLocked;
	}
	else if (!ActiveBlockers.IsEmpty() && !bDeathRequest)
	{
		rejectionReason = LRGameplayTags::StateRejectBlocked;
	}
	else if (!LRStateRules::IsTransitionAllowed(CurrentMode, request)
		|| request.Source != LRStateRules::GetSourceTag(request.RequestType))
	{
		rejectionReason = LRGameplayTags::StateRejectInvalidTransition;
	}

	if (rejectionReason.IsValid())
	{
		result.Reason = rejectionReason;
		RejectRequest(request, rejectionReason);
		return result;
	}

	if (bDeathRequest && bPresentationLocked)
	{
		NotifyPresentationComplete();
	}

	const ELRPerceptionMode previousMode = CurrentMode;
	StartPresentationLock();
	OnStateChanging.Broadcast(previousMode, request.TargetMode, request.Source);
	CurrentMode = request.TargetMode;
	LastTransitionReason = request.Source;
	OnStateChanged.Broadcast(CurrentMode, request.Source);

	result.bAccepted = true;
	result.CurrentMode = CurrentMode;
	result.Reason = request.Source;
	UE_LOG(LogLostRunicState, Log, TEXT("Owner=%s state %d -> %d source=%s"), *GetNameSafe(GetOwner()),
		static_cast<int32>(previousMode), static_cast<int32>(CurrentMode), *request.Source.ToString());
	return result;
}

/**
 * @brief 记录一次闭眼或睁眼按下，按先按者优先规则确定目标状态，并使用调优时长启动一次性长按计时。
 * @param inputType 闭眼或睁眼输入语义，不包含具体键位。
 */
void ULRStateComponent::BeginEyeInput(const ELRStateRequestType inputType)
{
	if (!InputGate.Press(inputType))
	{
		FLRStateChangeRequest request;
		request.RequestType = inputType;
		request.Source = LRStateRules::GetSourceTag(inputType);
		request.TargetMode = CurrentMode;
		RejectRequest(request, LRGameplayTags::StateRejectConcurrentInput);
		return;
	}

	float holdSeconds = 0.0f;
	if (!LRStateRules::ResolveEyeTransition(CurrentMode, inputType, GetEffectiveTuning(), PendingInputTarget, holdSeconds)
		|| bPresentationLocked || !ActiveBlockers.IsEmpty())
	{
		FLRStateChangeRequest request;
		request.RequestType = inputType;
		request.Source = LRStateRules::GetSourceTag(inputType);
		request.TargetMode = PendingInputTarget;
		RejectRequest(request, bPresentationLocked ? LRGameplayTags::StateRejectPresentationLocked
			: !ActiveBlockers.IsEmpty() ? LRGameplayTags::StateRejectBlocked : LRGameplayTags::StateRejectInvalidTransition);
		InputGate.ConsumeThreshold(inputType);
		return;
	}

	OnHoldStarted.Broadcast(inputType, PendingInputTarget, holdSeconds);
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(HoldTimer, this, &ULRStateComponent::HandleHoldThreshold, holdSeconds, false);
	}
}

/**
 * @brief 结束指定眼部输入；未达到阈值时取消请求，已提交的同一次按下不会重复触发。
 * @param inputType 闭眼或睁眼输入语义，不包含具体键位。
 */
void ULRStateComponent::EndEyeInput(const ELRStateRequestType inputType)
{
	if (InputGate.GetActiveInput() == inputType && GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(HoldTimer);
	}
	if (InputGate.Release(inputType))
	{
		OnHoldCanceled.Broadcast(inputType);
	}
}

/**
 * @brief 取消当前眼部输入事务并清空门控，供菜单、对话、死亡或过场接管输入时调用。
 */
void ULRStateComponent::CancelEyeInputSequence()
{
	const ELRStateRequestType activeInput = InputGate.GetActiveInput();
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(HoldTimer);
	}
	InputGate.Reset();
	if (activeInput != ELRStateRequestType::None)
	{
		OnHoldCanceled.Broadcast(activeInput);
	}
}

/**
 * @brief 按来源标签启用或解除状态输入阻塞；多个系统可独立持有阻塞，互不覆盖。
 * @param blocker 标识菜单、对话、过场、死亡等输入阻塞来源的 Gameplay Tag。
 * @param bActive 为 true 时加入阻塞或启用状态，为 false 时移除或停用。
 */
void ULRStateComponent::SetBlockerActive(const FGameplayTag blocker, const bool bActive)
{
	if (!blocker.IsValid())
	{
		return;
	}
	if (bActive)
	{
		ActiveBlockers.AddTag(blocker);
		CancelEyeInputSequence();
	}
	else
	{
		ActiveBlockers.RemoveTag(blocker);
	}
}

/**
 * @brief 接收 UMG、动画或表现层的完成回调并释放状态锁；安全超时只处理资源异常。
 */
void ULRStateComponent::NotifyPresentationComplete()
{
	if (!bPresentationLocked)
	{
		return;
	}
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(PresentationSafetyTimer);
	}
	bPresentationLocked = false;
	OnPresentationLockChanged.Broadcast(false);
}

/**
 * @brief 向对应日志分类输出当前状态、配置来源和关键运行时值，供 LR.Debug 命令诊断。
 */
void ULRStateComponent::LogDiagnostics() const
{
	UE_LOG(LogLostRunicState, Display, TEXT("Owner=%s Mode=%d PresentationLocked=%s LastReason=%s Blockers=%s"),
		*GetNameSafe(GetOwner()), static_cast<int32>(CurrentMode), bPresentationLocked ? TEXT("true") : TEXT("false"),
		*LastTransitionReason.ToString(), *ActiveBlockers.ToStringSimple());
}

/**
 * @brief 在长按达到调优阈值时构造并提交唯一状态请求。
 */
void ULRStateComponent::HandleHoldThreshold()
{
	const ELRStateRequestType inputType = InputGate.GetActiveInput();
	if (!InputGate.ConsumeThreshold(inputType))
	{
		return;
	}
	OnHoldThresholdReached.Broadcast(inputType, PendingInputTarget);

	FLRStateChangeRequest request;
	request.TargetMode = PendingInputTarget;
	request.RequestType = inputType;
	request.Source = LRStateRules::GetSourceTag(inputType);
	RequestStateChange(request);
}

/**
 * @brief 在表现资源未回调时执行安全解锁并记录异常，避免永久阻塞输入。
 */
void ULRStateComponent::HandlePresentationSafetyTimeout()
{
	UE_LOG(LogLostRunicState, Warning, TEXT("Owner=%s presentation lock reached its %.2fs safety timeout."),
		*GetNameSafe(GetOwner()), GetEffectiveTuning().PresentationSafetyTimeoutSeconds);
	NotifyPresentationComplete();
}

/**
 * @brief 统一构造状态拒绝结果、记录原因标签并广播 Rejected 事件。
 * @param request 不可变领域请求，包含本次操作所需的稳定 ID、来源、目标或原因。
 * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
 */
void ULRStateComponent::RejectRequest(const FLRStateChangeRequest& request, const FGameplayTag reason)
{
	OnStateChangeRejected.Broadcast(request, reason);
	UE_LOG(LogLostRunicState, Verbose, TEXT("Owner=%s rejected target=%d request=%d reason=%s"), *GetNameSafe(GetOwner()),
		static_cast<int32>(request.TargetMode), static_cast<int32>(request.RequestType), *reason.ToString());
}

/**
 * @brief 开始 Start Presentation Lock 流程，建立本次操作拥有的状态、委托或计时器。
 */
void ULRStateComponent::StartPresentationLock()
{
	bPresentationLocked = true;
	OnPresentationLockChanged.Broadcast(true);
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(PresentationSafetyTimer, this,
			&ULRStateComponent::HandlePresentationSafetyTimeout, GetEffectiveTuning().PresentationSafetyTimeoutSeconds, false);
	}
}

/**
 * @brief 查询 Effective Tuning；不修改领域状态。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
const ULRStateTuning& ULRStateComponent::GetEffectiveTuning() const
{
	return Tuning ? *Tuning : *GetDefault<ULRStateTuning>();
}
