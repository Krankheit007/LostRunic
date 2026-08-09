/**
 * @file LRStateRules.cpp
 * @brief 提供无 UObject 依赖的四状态合法性矩阵、长按目标和输入门控纯规则，供运行时组件与 LostRunic.State 自动化测试共同调用。
 *
 * 关联文件：LRStateRules.h；所属领域：State。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "State/LRStateRules.h"

#include "Core/LRGameplayTags.h"
#include "Data/LRStateTuning.h"

/**
 * @brief 记录闭眼或睁眼按下；若另一眼部输入已持有，则按先按者优先拒绝。
 * @param inputType 闭眼或睁眼输入语义，不包含具体键位。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool FLRStateInputGate::Press(const ELRStateRequestType inputType)
{
	if (inputType != ELRStateRequestType::CloseEyes && inputType != ELRStateRequestType::OpenEyes)
	{
		return false;
	}
	if (IsHeld(inputType))
	{
		return false;
	}

	SetHeld(inputType, true);
	if (ActiveInput != ELRStateRequestType::None || bWaitForAllReleased)
	{
		bWaitForAllReleased = true;
		return false;
	}

	ActiveInput = inputType;
	bThresholdConsumed = false;
	return true;
}

/**
 * @brief 记录眼部输入释放；阈值前释放时取消，阈值后仅清理本次按下。
 * @param inputType 闭眼或睁眼输入语义，不包含具体键位。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool FLRStateInputGate::Release(const ELRStateRequestType inputType)
{
	SetHeld(inputType, false);
	const bool bCanceled = ActiveInput == inputType && !bThresholdConsumed;
	if (ActiveInput == inputType)
	{
		ActiveInput = ELRStateRequestType::None;
		bThresholdConsumed = false;
	}
	if (!AreAnyHeld())
	{
		bWaitForAllReleased = false;
	}
	return bCanceled;
}

/**
 * @brief 在本次物理按下首次达到长按阈值时返回目标状态，并阻止同次按下重复提交。
 * @param inputType 闭眼或睁眼输入语义，不包含具体键位。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool FLRStateInputGate::ConsumeThreshold(const ELRStateRequestType inputType)
{
	if (ActiveInput != inputType || !IsHeld(inputType) || bThresholdConsumed)
	{
		return false;
	}
	bThresholdConsumed = true;
	return true;
}

/**
 * @brief 实现 Reset 对应的领域步骤；调用关系和状态所有权由本文件所属系统维护。
 */
void FLRStateInputGate::Reset()
{
	ActiveInput = ELRStateRequestType::None;
	bCloseEyesHeld = false;
	bOpenEyesHeld = false;
	bThresholdConsumed = false;
	bWaitForAllReleased = false;
}

/**
 * @brief 判断 Is Held 对应条件；不产生玩法副作用。
 * @param inputType 闭眼或睁眼输入语义，不包含具体键位。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool FLRStateInputGate::IsHeld(const ELRStateRequestType inputType) const
{
	return inputType == ELRStateRequestType::CloseEyes ? bCloseEyesHeld
		: inputType == ELRStateRequestType::OpenEyes && bOpenEyesHeld;
}

/**
 * @brief 更新 Held，并在需要时同步组件状态或广播变化事件。
 * @param inputType 闭眼或睁眼输入语义，不包含具体键位。
 * @param bHeld 布尔开关 `bHeld`；true 表示启用或条件成立，false 表示禁用或条件不成立。
 */
void FLRStateInputGate::SetHeld(const ELRStateRequestType inputType, const bool bHeld)
{
	if (inputType == ELRStateRequestType::CloseEyes)
	{
		bCloseEyesHeld = bHeld;
	}
	else if (inputType == ELRStateRequestType::OpenEyes)
	{
		bOpenEyesHeld = bHeld;
	}
}

/**
 * @brief 判断 Is Transition Allowed 对应条件；不产生玩法副作用。
 * @param currentMode 本次操作使用的 `currentMode` 枚举或模式值。
 * @param request 不可变领域请求，包含本次操作所需的稳定 ID、来源、目标或原因。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool LRStateRules::IsTransitionAllowed(const ELRPerceptionMode currentMode, const FLRStateChangeRequest& request)
{
	if (currentMode == request.TargetMode)
	{
		return false;
	}
	if (request.RequestType == ELRStateRequestType::Death)
	{
		return currentMode != ELRPerceptionMode::Memory && request.TargetMode == ELRPerceptionMode::Memory;
	}
	if (request.RequestType == ELRStateRequestType::Narrative)
	{
		return (currentMode == ELRPerceptionMode::Perception && request.TargetMode == ELRPerceptionMode::Memory)
			|| (currentMode == ELRPerceptionMode::Memory && request.TargetMode == ELRPerceptionMode::Normal);
	}
	if (request.RequestType == ELRStateRequestType::CloseEyes)
	{
		return (currentMode == ELRPerceptionMode::Normal && request.TargetMode == ELRPerceptionMode::Perception)
			|| (currentMode == ELRPerceptionMode::Courage && request.TargetMode == ELRPerceptionMode::Normal);
	}
	if (request.RequestType == ELRStateRequestType::OpenEyes)
	{
		return (currentMode == ELRPerceptionMode::Normal && request.TargetMode == ELRPerceptionMode::Courage)
			|| (currentMode == ELRPerceptionMode::Perception && request.TargetMode == ELRPerceptionMode::Normal);
	}
	return false;
}

/**
 * @brief 执行 Resolve Eye Transition 的纯规则或事务判定，失败时提供结构化原因。
 * @param currentMode 本次操作使用的 `currentMode` 枚举或模式值。
 * @param inputType 闭眼或睁眼输入语义，不包含具体键位。
 * @param tuning 数据或调优来源 `tuning`；调用期间只读，并按稳定 ID 解析内容。
 * @param outTargetMode 本次操作使用的 `outTargetMode` 枚举或模式值。
 * @param outHoldSeconds 时间值 `outHoldSeconds`，单位为秒。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool LRStateRules::ResolveEyeTransition(const ELRPerceptionMode currentMode, const ELRStateRequestType inputType,
	const ULRStateTuning& tuning, ELRPerceptionMode& outTargetMode, float& outHoldSeconds)
{
	if (currentMode == ELRPerceptionMode::Normal && inputType == ELRStateRequestType::CloseEyes)
	{
		outTargetMode = ELRPerceptionMode::Perception;
		outHoldSeconds = tuning.EnterHoldSeconds;
		return true;
	}
	if (currentMode == ELRPerceptionMode::Normal && inputType == ELRStateRequestType::OpenEyes)
	{
		outTargetMode = ELRPerceptionMode::Courage;
		outHoldSeconds = tuning.EnterHoldSeconds;
		return true;
	}
	if ((currentMode == ELRPerceptionMode::Perception && inputType == ELRStateRequestType::OpenEyes)
		|| (currentMode == ELRPerceptionMode::Courage && inputType == ELRStateRequestType::CloseEyes))
	{
		outTargetMode = ELRPerceptionMode::Normal;
		outHoldSeconds = tuning.ExitHoldSeconds;
		return true;
	}
	return false;
}

/**
 * @brief 查询 Source Tag；不修改领域状态。
 * @param requestType 本次操作使用的 `requestType` 枚举或模式值。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
FGameplayTag LRStateRules::GetSourceTag(const ELRStateRequestType requestType)
{
	if (requestType == ELRStateRequestType::CloseEyes)
	{
		return LRGameplayTags::StateSourceInputCloseEyes;
	}
	if (requestType == ELRStateRequestType::OpenEyes)
	{
		return LRGameplayTags::StateSourceInputOpenEyes;
	}
	if (requestType == ELRStateRequestType::Death)
	{
		return LRGameplayTags::StateSourceDeath;
	}
	if (requestType == ELRStateRequestType::Narrative)
	{
		return LRGameplayTags::StateSourceNarrative;
	}
	return FGameplayTag();
}
