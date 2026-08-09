/**
 * @file LRStatePresentationComponent.h
 * @brief 实现 Normal、Perception、Courage、Memory 四状态请求边界、长按输入事务和表现锁。输入、剧情与死亡只能提交请求，不能直接改写当前状态。
 *
 * 关联文件：LRStatePresentationComponent.cpp；所属领域：State。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Components/ActorComponent.h"
#include "Core/LRTypes.h"
#include "GameplayTagContainer.h"

#include "LRStatePresentationComponent.generated.h"

class ULRStateComponent;

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(ClassGroup = "Lost Runic", BlueprintType, meta = (BlueprintSpawnableComponent, DisplayName = "Lost Runic State Presentation"))
class LOSTRUNIC_API ULRStatePresentationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 */
	ULRStatePresentationComponent();

	/**
	 * @brief 在进入世界后解析运行时依赖、绑定事件并启动所需计时器；构造阶段不访问 World 或玩家对象。
	 */
	virtual void BeginPlay() override;
	/**
	 * @brief 解除委托并清理计时器或缓存，避免关卡切换和对象销毁后继续收到回调。
	 * @param endPlayReason Unreal 提供的结束原因，用于区分销毁、关卡切换和退出。
	 */
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

	/**
	 * @brief 由状态遮罩或动画完成事件释放表现锁，不参与状态合法性判定。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|State|Presentation")
	void CompleteStatePresentation();

protected:
	/**
	 * @brief 把心理状态变化映射到蓝图表现事件，并等待完成回调释放状态锁。
	 * @param previousMode 本次操作使用的 `previousMode` 枚举或模式值。
	 * @param nextMode 本次操作使用的 `nextMode` 枚举或模式值。
	 * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Lost Runic|State|Presentation", meta = (DisplayName = "Present State Change"))
	void PresentStateChange(ELRPerceptionMode previousMode, ELRPerceptionMode nextMode, FGameplayTag reason);

private:
	/**
	 * @brief 处理 Handle State Changing 事件，将引擎回调转换为对应领域状态更新。
	 * @param previousMode 本次操作使用的 `previousMode` 枚举或模式值。
	 * @param nextMode 本次操作使用的 `nextMode` 枚举或模式值。
	 * @param reason Gameplay Tag 原因，用于状态转换、日志和自动化测试追踪。
	 */
	UFUNCTION()
	void HandleStateChanging(ELRPerceptionMode previousMode, ELRPerceptionMode nextMode, FGameplayTag reason);

	/** State Component 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TObjectPtr<ULRStateComponent> StateComponent;
};
