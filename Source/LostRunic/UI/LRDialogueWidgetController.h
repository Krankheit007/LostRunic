/**
 * @file LRDialogueWidgetController.h
 * @brief 实现 HUD、状态遮罩、对话/阅读、背包/笔记/收藏、暂停、存档槽和过场的控制器边界。UI 订阅领域事件并负责表现，不参与核心规则判定。
 *
 * 关联文件：LRDialogueWidgetController.cpp；所属领域：UI。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "Narrative/LRNarrativeTypes.h"
#include "UI/LRUITypes.h"
#include "UObject/Object.h"

#include "LRDialogueWidgetController.generated.h"

class ULRDialogueSubsystem;
class ULRUITuning;
class UWorld;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLRNarrativePresentationChanged, FLRNarrativePresentation, presentation);

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
UCLASS(BlueprintType, meta = (DisplayName = "Lost Runic Dialogue Widget Controller"))
class LOSTRUNIC_API ULRDialogueWidgetController : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * @brief 初始化子系统拥有的长期状态与事件绑定。
	 * @param dialogueSubsystem 调用方提供的 `dialogueSubsystem`，只在本次操作范围内使用。
	 * @param tuning 数据或调优来源 `tuning`；调用期间只读，并按稳定 ID 解析内容。
	 * @param world 要解析地图 ID、应用恢复状态或执行查询的 Unreal World。
	 */
	void Initialize(ULRDialogueSubsystem* dialogueSubsystem, ULRUITuning* tuning, UWorld* world);
	/**
	 * @brief 释放子系统事件绑定和运行时缓存。
	 */
	void Deinitialize();

	/**
	 * @brief 处理 Handle Confirm 事件，将引擎回调转换为对应领域状态更新。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Narrative")
	FLRNarrativeResult HandleConfirm();

	/**
	 * @brief 执行 Select Choice 的纯规则或事务判定，失败时提供结构化原因。
	 * @param choiceId 稳定标识 `choiceId`；用于内容查询和存档，不依赖显示名或数组序号。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Narrative")
	FLRNarrativeResult SelectChoice(FName choiceId);

	/**
	 * @brief 结束或取消 End Session 流程，并清理本次操作拥有的临时状态。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lost Runic|Narrative")
	void EndSession();

	/**
	 * @brief 查询 Presentation；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	UFUNCTION(BlueprintPure, Category = "Lost Runic|Narrative")
	FLRNarrativePresentation GetPresentation() const { return Presentation; }

	/**
	 * @brief 立即显示当前叙事页面全文；下一次确认才推进到下一行。
	 */
	void RevealCurrentText();
	/**
	 * @brief 实现 Advance Typewriter For Test 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 * @param elapsedSeconds 时间值 `elapsedSeconds`，单位为秒。
	 */
	void AdvanceTypewriterForTest(float elapsedSeconds);

	/** 当 Presentation Changed 发生时广播；蓝图可绑定该委托以更新表现，不应在回调中改写核心规则。  */
	UPROPERTY(BlueprintAssignable, Category = "Lost Runic|Narrative")
	FLRNarrativePresentationChanged OnPresentationChanged;

private:
	/**
	 * @brief 处理 Handle Page Changed 事件，将引擎回调转换为对应领域状态更新。
	 * @param page 本次领域操作的结构化数据 `page`；字段语义由对应 USTRUCT 定义。
	 */
	UFUNCTION()
	void HandlePageChanged(FLRNarrativePage page);

	/**
	 * @brief 处理 Handle Session Ended 事件，将引擎回调转换为对应领域状态更新。
	 * @param sessionType 本次操作使用的 `sessionType` 枚举或模式值。
	 * @param finalContentId 稳定标识 `finalContentId`；用于内容查询和存档，不依赖显示名或数组序号。
	 */
	UFUNCTION()
	void HandleSessionEnded(ELRNarrativeSessionType sessionType, FName finalContentId);

	/**
	 * @brief 根据 UI 调优速度刷新当前可见字符数；该计时器仅控制表现，不决定叙事推进。
	 */
	void RefreshTypewriter();
	/**
	 * @brief 根据最新领域状态刷新 Update Visible Characters，并仅在值变化时通知订阅者。
	 * @param visibleCharacterCount 本次操作使用的计数、增量或索引 `visibleCharacterCount`；由函数校验合法范围。
	 */
	void UpdateVisibleCharacters(int32 visibleCharacterCount);
	/**
	 * @brief 结束或取消 Stop Typewriter 流程，并清理本次操作拥有的临时状态。
	 */
	void StopTypewriter();
	/**
	 * @brief 查询 Characters Per Second；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	float GetCharactersPerSecond() const;
	/**
	 * @brief 查询 Update Seconds；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	float GetUpdateSeconds() const;

	/** Dialogue Subsystem 的领域数据，由所属类型负责维护和校验。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TObjectPtr<ULRDialogueSubsystem> DialogueSubsystem;

	/** 运行时解析出的调优资产缓存；不序列化，不由蓝图编辑。 该字段仅为运行时缓存，不进入存档。 */
	UPROPERTY(Transient)
	TObjectPtr<ULRUITuning> Tuning;

	/** World 的内部运行时数据；不参与蓝图配置。 */
	TWeakObjectPtr<UWorld> World;
	/** Presentation 的内部运行时数据；不参与蓝图配置。 */
	FLRNarrativePresentation Presentation;
	/** Full Text 的内部运行时数据；不参与蓝图配置。 */
	FString FullText;
	/** Page Started At Seconds 的内部运行时数据；不参与蓝图配置。 */
	double PageStartedAtSeconds = 0.0;
	/** Typewriter Timer 的运行时句柄，用于取消回调并避免 Tick；不在蓝图中配置。 */
	FTimerHandle TypewriterTimer;
};
