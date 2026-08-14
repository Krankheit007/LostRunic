/**
 * @file LRStatePresentationComponent.cpp
 * @brief 实现 Normal、Perception、Courage、Memory 四状态请求边界、长按输入事务和表现锁。输入、剧情与死亡只能提交请求，不能直接改写当前状态。
 *
 * 关联文件：LRStatePresentationComponent.h；所属领域：State。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "State/LRStatePresentationComponent.h"

#include "Data/LRGameTuningSet.h"
#include "Data/LRPresentationTuning.h"
#include "Engine/GameInstance.h"
#include "Framework/LRGameInstanceSubsystem.h"
#include "State/LRStateComponent.h"

/**
 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
 */
ULRStatePresentationComponent::ULRStatePresentationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

/**
 * @brief 在进入世界后解析运行时依赖、绑定事件并启动所需计时器；构造阶段不访问 World 或玩家对象。
 */
void ULRStatePresentationComponent::BeginPlay()
{
	Super::BeginPlay();
	StateComponent = GetOwner() ? GetOwner()->FindComponentByClass<ULRStateComponent>() : nullptr;
	const UGameInstance* gameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const ULRGameInstanceSubsystem* subsystem = gameInstance ? gameInstance->GetSubsystem<ULRGameInstanceSubsystem>() : nullptr;
	Tuning = subsystem && subsystem->GetTuningSet() ? subsystem->GetTuningSet()->Presentation : nullptr;
	if (ensureMsgf(StateComponent, TEXT("%s requires a sibling LRStateComponent."), *GetNameSafe(this)))
	{
		StateComponent->OnStateChanging.AddDynamic(this, &ULRStatePresentationComponent::HandleStateChanging);
	}
}

/**
 * @brief 查询 Perception Reveal Radius（角色周围显现半径，设计 4.5m）；艺术表现预留接入点。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
float ULRStatePresentationComponent::GetPerceptionRevealRadius() const
{
	return Tuning ? Tuning->PerceptionRevealRadius : GetDefault<ULRPresentationTuning>()->PerceptionRevealRadius;
}

/**
 * @brief 查询 Noise Reveal Radius（声源周围显现半径，设计 2m）；艺术表现预留接入点。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
float ULRStatePresentationComponent::GetNoiseRevealRadius() const
{
	return Tuning ? Tuning->NoiseRevealRadius : GetDefault<ULRPresentationTuning>()->NoiseRevealRadius;
}

/**
 * @brief 查询 Noise Reveal Duration Seconds（声源显现时长，设计 5s）；艺术表现预留接入点。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
float ULRStatePresentationComponent::GetNoiseRevealDurationSeconds() const
{
	return Tuning ? Tuning->NoiseRevealDurationSeconds : GetDefault<ULRPresentationTuning>()->NoiseRevealDurationSeconds;
}

/**
 * @brief 查询 Perception Blend Weight（感知后处理混合权重）；艺术表现预留接入点。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
float ULRStatePresentationComponent::GetPerceptionBlendWeight() const
{
	return Tuning ? Tuning->PerceptionBlendWeight : GetDefault<ULRPresentationTuning>()->PerceptionBlendWeight;
}

/**
 * @brief 查询 Courage Blend Weight（勇气后处理混合权重）；艺术表现预留接入点。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
float ULRStatePresentationComponent::GetCourageBlendWeight() const
{
	return Tuning ? Tuning->CourageBlendWeight : GetDefault<ULRPresentationTuning>()->CourageBlendWeight;
}

/**
 * @brief 解除委托并清理计时器或缓存，避免关卡切换和对象销毁后继续收到回调。
 * @param endPlayReason Unreal 提供的结束原因，用于区分销毁、关卡切换和退出。
 */
void ULRStatePresentationComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	if (StateComponent)
	{
		StateComponent->OnStateChanging.RemoveDynamic(this, &ULRStatePresentationComponent::HandleStateChanging);
	}
	Super::EndPlay(endPlayReason);
}

/**
 * @brief 由状态遮罩或动画完成事件释放表现锁，不参与状态合法性判定。
 */
void ULRStatePresentationComponent::CompleteStatePresentation()
{
	if (StateComponent)
	{
		StateComponent->NotifyPresentationComplete();
	}
}

/**
 * @brief 处理 Handle State Changing 事件，将引擎回调转换为对应领域状态更新。
 * @param previousMode 本次操作使用的 `previousMode` 枚举或模式值。
 * @param nextMode 本次操作使用的 `nextMode` 枚举或模式值。
 * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
 */
void ULRStatePresentationComponent::HandleStateChanging(const ELRPerceptionMode previousMode,
	const ELRPerceptionMode nextMode, const FGameplayTag reason)
{
	OnStatePresentationRequested.Broadcast(previousMode, nextMode, reason);
	PresentStateChange(previousMode, nextMode, reason);
}
