/**
 * @file LRStateRules.h
 * @brief 提供无 UObject 依赖的四状态合法性矩阵、长按目标和输入门控纯规则，供运行时组件与 LostRunic.State 自动化测试共同调用。
 *
 * 关联文件：LRStateRules.cpp；所属领域：State。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#pragma once

#include "State/LRStateTypes.h"

class ULRStateTuning;

/** 该公开类型定义本文件领域边界的数据或行为；具体字段、参数与约束见下方中文注释。 */
class LOSTRUNIC_API FLRStateInputGate
{
public:
	/**
	 * @brief 记录闭眼或睁眼按下；若另一眼部输入已持有，则按先按者优先拒绝。
	 * @param inputType 闭眼或睁眼输入语义，不包含具体键位。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool Press(ELRStateRequestType inputType);
	/**
	 * @brief 记录眼部输入释放；阈值前释放时取消，阈值后仅清理本次按下。
	 * @param inputType 闭眼或睁眼输入语义，不包含具体键位。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool Release(ELRStateRequestType inputType);
	/**
	 * @brief 在本次物理按下首次达到长按阈值时返回目标状态，并阻止同次按下重复提交。
	 * @param inputType 闭眼或睁眼输入语义，不包含具体键位。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool ConsumeThreshold(ELRStateRequestType inputType);
	/**
	 * @brief 实现 Reset 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 */
	void Reset();

	/**
	 * @brief 查询 Active Input；不修改领域状态。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	ELRStateRequestType GetActiveInput() const { return ActiveInput; }
	/**
	 * @brief 判断 Is Waiting For All Released 对应条件；不产生玩法副作用。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool IsWaitingForAllReleased() const { return bWaitForAllReleased; }

private:
	/**
	 * @brief 判断 Is Held 对应条件；不产生玩法副作用。
	 * @param inputType 闭眼或睁眼输入语义，不包含具体键位。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool IsHeld(ELRStateRequestType inputType) const;
	/**
	 * @brief 更新 Held，并在需要时同步组件状态或广播变化事件。
	 * @param inputType 闭眼或睁眼输入语义，不包含具体键位。
	 * @param bHeld 布尔开关 `bHeld`；true 表示启用或条件成立，false 表示禁用或条件不成立。
	 */
	void SetHeld(ELRStateRequestType inputType, bool bHeld);
	/**
	 * @brief 实现 Are Any Held 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool AreAnyHeld() const { return bCloseEyesHeld || bOpenEyesHeld; }

	/** Active Input 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	ELRStateRequestType ActiveInput = ELRStateRequestType::None;
	/** Close Eyes Held 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	bool bCloseEyesHeld = false;
	/** Open Eyes Held 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	bool bOpenEyesHeld = false;
	/** Threshold Consumed 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	bool bThresholdConsumed = false;
	/** Wait For All Released 的运行时状态；由所属类型维护，不在蓝图中配置。 */
	bool bWaitForAllReleased = false;
};

namespace LRStateRules
{
	/**
	 * @brief 判断 Is Transition Allowed 对应条件；不产生玩法副作用。
	 * @param currentMode 本次操作使用的 `currentMode` 枚举或模式值。
	 * @param request 不可变领域请求，包含本次操作所需的稳定 ID、来源、目标或原因。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	LOSTRUNIC_API bool IsTransitionAllowed(ELRPerceptionMode currentMode, const FLRStateChangeRequest& request);
	LOSTRUNIC_API bool ResolveEyeTransition(ELRPerceptionMode currentMode, ELRStateRequestType inputType,
		const ULRStateTuning& tuning, ELRPerceptionMode& outTargetMode, float& outHoldSeconds);
	/**
	 * @brief 查询 Source Tag；不修改领域状态。
	 * @param requestType 本次操作使用的 `requestType` 枚举或模式值。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	LOSTRUNIC_API FGameplayTag GetSourceTag(ELRStateRequestType requestType);
}
