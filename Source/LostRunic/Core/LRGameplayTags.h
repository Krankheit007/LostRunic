/**
 * @file LRGameplayTags.h
 * @brief 声明 LostRunic 各玩法领域共享的稳定 ID、Gameplay Tags、日志分类、数据校验与调试命令，供状态、交互、AI、叙事和存档系统统一使用。
 *
 * 关联文件：LRGameplayTags.cpp；所属领域：Core。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "NativeGameplayTags.h"

namespace LRGameplayTags
{
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param StateSourceInputCloseEyes 调用方提供的 `StateSourceInputCloseEyes`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(StateSourceInputCloseEyes);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param StateSourceInputOpenEyes 调用方提供的 `StateSourceInputOpenEyes`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(StateSourceInputOpenEyes);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param StateSourceDeath 调用方提供的 `StateSourceDeath`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(StateSourceDeath);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param StateSourceNarrative 调用方提供的 `StateSourceNarrative`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(StateSourceNarrative);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param StateBlockerHidden 调用方提供的 `StateBlockerHidden`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(StateBlockerHidden);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param StateBlockerDialogue 调用方提供的 `StateBlockerDialogue`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(StateBlockerDialogue);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param StateBlockerMenu 调用方提供的 `StateBlockerMenu`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(StateBlockerMenu);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param StateBlockerTransition 本次领域操作的结构化数据 `StateBlockerTransition`；字段语义由对应 USTRUCT 定义。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(StateBlockerTransition);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param StateBlockerPresentation 本次领域操作的结构化数据 `StateBlockerPresentation`；字段语义由对应 USTRUCT 定义。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(StateBlockerPresentation);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param StateBlockerDeath 调用方提供的 `StateBlockerDeath`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(StateBlockerDeath);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param StateRejectInvalidTransition 本次领域操作的结构化数据 `StateRejectInvalidTransition`；字段语义由对应 USTRUCT 定义。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(StateRejectInvalidTransition);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param StateRejectBlocked 调用方提供的 `StateRejectBlocked`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(StateRejectBlocked);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param StateRejectConcurrentInput 输入动作或数值 `StateRejectConcurrentInput`；不包含写死的具体键位。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(StateRejectConcurrentInput);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param StateRejectPresentationLocked 调用方提供的 `StateRejectPresentationLocked`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(StateRejectPresentationLocked);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param StateRejectAlreadyCurrent 调用方提供的 `StateRejectAlreadyCurrent`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(StateRejectAlreadyCurrent);

	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param InteractionActionInteract 调用方提供的 `InteractionActionInteract`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InteractionActionInteract);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param InteractionActionPickup 调用方提供的 `InteractionActionPickup`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InteractionActionPickup);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param InteractionActionRead 调用方提供的 `InteractionActionRead`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InteractionActionRead);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param InteractionActionTalk 调用方提供的 `InteractionActionTalk`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InteractionActionTalk);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param InteractionActionUse 调用方提供的 `InteractionActionUse`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InteractionActionUse);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param InteractionActionHide 调用方提供的 `InteractionActionHide`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InteractionActionHide);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param InteractionRejectNoTarget 参与本次操作的运行时对象 `InteractionRejectNoTarget`；函数会检查空值和所需接口。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InteractionRejectNoTarget);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param InteractionRejectTooFar 调用方提供的 `InteractionRejectTooFar`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InteractionRejectTooFar);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param InteractionRejectWrongFacing 调用方提供的 `InteractionRejectWrongFacing`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InteractionRejectWrongFacing);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param InteractionRejectOccluded 调用方提供的 `InteractionRejectOccluded`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InteractionRejectOccluded);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param InteractionRejectState 本次操作使用的 `InteractionRejectState` 枚举或模式值。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InteractionRejectState);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param InteractionRejectItem 调用方提供的 `InteractionRejectItem`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InteractionRejectItem);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param InteractionRejectCompleted 调用方提供的 `InteractionRejectCompleted`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InteractionRejectCompleted);

	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param ItemCategoryKey 调用方提供的 `ItemCategoryKey`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(ItemCategoryKey);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param ItemCategoryCourageWeapon 调用方提供的 `ItemCategoryCourageWeapon`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(ItemCategoryCourageWeapon);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param ItemUseRejectNotOwned 调用方提供的 `ItemUseRejectNotOwned`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(ItemUseRejectNotOwned);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param ItemUseRejectInvalidSlot 本次操作使用的计数、增量或索引 `ItemUseRejectInvalidSlot`；由函数校验合法范围。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(ItemUseRejectInvalidSlot);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param ItemUseRejectCooldown 时间值 `ItemUseRejectCooldown`，单位为秒。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(ItemUseRejectCooldown);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param ItemUseRejectImmune 调用方提供的 `ItemUseRejectImmune`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(ItemUseRejectImmune);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param ItemUseRejectTarget 参与本次操作的运行时对象 `ItemUseRejectTarget`；函数会检查空值和所需接口。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(ItemUseRejectTarget);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param ItemUseRejectExecution 调用方提供的 `ItemUseRejectExecution`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(ItemUseRejectExecution);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param NoiseFootstepWalk 调用方提供的 `NoiseFootstepWalk`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(NoiseFootstepWalk);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param NoiseFootstepRun 调用方提供的 `NoiseFootstepRun`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(NoiseFootstepRun);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param NoiseInteraction 输入动作或数值 `NoiseInteraction`；不包含写死的具体键位。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(NoiseInteraction);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param SightPlayer 调用方提供的 `SightPlayer`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SightPlayer);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param SightPlayerLost 调用方提供的 `SightPlayerLost`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SightPlayerLost);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param SearchReached 调用方提供的 `SearchReached`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SearchReached);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param SearchAlertDecay 调用方提供的 `SearchAlertDecay`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SearchAlertDecay);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param SearchTimeout 调用方提供的 `SearchTimeout`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SearchTimeout);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param AIEventAlertChanged 调用方提供的 `AIEventAlertChanged`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(AIEventAlertChanged);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param NarrativeEventCompleted 调用方提供的 `NarrativeEventCompleted`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(NarrativeEventCompleted);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param NarrativeRejectNoSession 调用方提供的 `NarrativeRejectNoSession`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(NarrativeRejectNoSession);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param NarrativeRejectMissingContent 调用方提供的 `NarrativeRejectMissingContent`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(NarrativeRejectMissingContent);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param NarrativeRejectConditions 调用方提供的 `NarrativeRejectConditions`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(NarrativeRejectConditions);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param NarrativeRejectInvalidChoice 调用方提供的 `NarrativeRejectInvalidChoice`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(NarrativeRejectInvalidChoice);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param NarrativeRejectAlreadyCompleted 调用方提供的 `NarrativeRejectAlreadyCompleted`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(NarrativeRejectAlreadyCompleted);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param SavePolicyAutoOnComplete 调用方提供的 `SavePolicyAutoOnComplete`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SavePolicyAutoOnComplete);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param SavePolicyCritical 调用方提供的 `SavePolicyCritical`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SavePolicyCritical);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param TargetDoorHomeKey 调用方提供的 `TargetDoorHomeKey`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TargetDoorHomeKey);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param TargetGuardCourageVulnerable 调用方提供的 `TargetGuardCourageVulnerable`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TargetGuardCourageVulnerable);
	/**
	 * @brief 创建对象并设置默认子对象、能力开关和安全初值；需要 World、资产或玩家的依赖延迟到初始化阶段解析。
	 * @param TargetGuardCourageImmune 调用方提供的 `TargetGuardCourageImmune`，只在本次操作范围内使用。
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TargetGuardCourageImmune);
}
