/**
 * @file LRInputConfig.cpp
 * @brief 定义 Enhanced Input 的语义资产集合和 Gameplay、Dialogue、Menu、Transition 上下文，具体键鼠与手柄按键由输入资产配置，C++ 只绑定动作语义。
 *
 * 关联文件：LRInputConfig.h；所属领域：Input。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Input/LRInputConfig.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

/**
 * @brief 校验当前资产的必填引用、数值边界及跨字段关系，并输出可诊断错误。
 * @param outError 输出校验失败原因；成功时保持为空。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRInputConfig::Validate(FString& outError) const
{
	if (!GameplayContext || !DialogueContext || !MenuContext || !TransitionContext)
	{
		outError = TEXT("All four input mapping contexts are required.");
		return false;
	}
	if (!MoveAction || !SneakAction || !RunAction || !InteractAction || !CloseEyesAction || !OpenEyesAction)
	{
		outError = TEXT("Gameplay movement, interaction, and state actions are required.");
		return false;
	}
	if (!ConfirmAction || !CancelAction || !AttackAction
		|| !ToggleCrouchAction || !OpenJournalAction || !PauseAction)
	{
		outError = TEXT("UI, gameplay, and attack actions are required.");
		return false;
	}
	return true;
}

#if WITH_EDITOR
/**
 * @brief 接入 Unreal Data Validation，将领域校验错误报告给编辑器。
 * @param context 用于本次条件匹配的 `context` 标签或上下文。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
EDataValidationResult ULRInputConfig::IsDataValid(FDataValidationContext& context) const
{
	FString error;
	if (!Validate(error))
	{
		context.AddError(FText::FromString(error));
		return EDataValidationResult::Invalid;
	}
	return Super::IsDataValid(context);
}
#endif
