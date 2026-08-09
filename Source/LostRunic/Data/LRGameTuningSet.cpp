/**
 * @file LRGameTuningSet.cpp
 * @brief 聚合 State、Movement、Interaction、Guard、Save、UI、Presentation 七类调优资产，并统一校验与输出实际来源。
 *
 * 关联文件：LRGameTuningSet.h；所属领域：Data。
 * 设计依据：Docs/Design/01_GameDesignSummary.md 与 Docs/Technical/04_TechnicalDesign.md。
 * 除带 EditDefaultsOnly、EditAnywhere 或 EditInstanceOnly 的字段外，其余成员均为运行时状态，不应由蓝图直接改写。
 */
#include "Data/LRGameTuningSet.h"

#include "Core/LRLog.h"
#include "Data/LRGuardTuning.h"
#include "Data/LRInteractionTuning.h"
#include "Data/LRMovementTuning.h"
#include "Data/LRPresentationTuning.h"
#include "Data/LRSaveTuning.h"
#include "Data/LRStateTuning.h"
#include "Data/LRUITuning.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

namespace
{
	template <typename T>
	/**
	 * @brief 执行 Validate Entry 的纯规则或事务判定，失败时提供结构化原因。
	 * @param label 调用方提供的 `label`，只在本次操作范围内使用。
	 * @param tuning 数据或调优来源 `tuning`；调用期间只读，并按稳定 ID 解析内容。
	 * @param outError 输出校验失败原因；成功时保持为空。
	 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
	 */
	bool ValidateEntry(const TCHAR* label, const T* tuning, FString& outError)
	{
		if (!tuning)
		{
			outError = FString::Printf(TEXT("Tuning set is missing required %s asset."), label);
			return false;
		}

		FString entryError;
		if (!tuning->Validate(entryError))
		{
			outError = FString::Printf(TEXT("%s tuning is invalid: %s"), label, *entryError);
			return false;
		}

		return true;
	}
}

/**
 * @brief 校验当前资产的必填引用、数值边界及跨字段关系，并输出可诊断错误。
 * @param outError 输出校验失败原因；成功时保持为空。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
bool ULRGameTuningSet::Validate(FString& outError) const
{
	return ValidateEntry(TEXT("State"), State.Get(), outError)
		&& ValidateEntry(TEXT("Movement"), Movement.Get(), outError)
		&& ValidateEntry(TEXT("Interaction"), Interaction.Get(), outError)
		&& ValidateEntry(TEXT("Guard"), Guard.Get(), outError)
		&& ValidateEntry(TEXT("Save"), Save.Get(), outError)
		&& ValidateEntry(TEXT("UI"), UI.Get(), outError)
		&& ValidateEntry(TEXT("Presentation"), Presentation.Get(), outError);
}

/**
 * @brief 输出聚合调优资产及 State、Movement、Interaction、Guard、Save、UI、Presentation 的实际来源。
 */
void ULRGameTuningSet::LogSources() const
{
	UE_LOG(LogLostRunicTuning, Display, TEXT("TuningSet=%s State=%s Movement=%s Interaction=%s Guard=%s Save=%s UI=%s Presentation=%s"),
		*GetPathName(), *GetNameSafe(State), *GetNameSafe(Movement), *GetNameSafe(Interaction), *GetNameSafe(Guard),
		*GetNameSafe(Save), *GetNameSafe(UI), *GetNameSafe(Presentation));
}

#if WITH_EDITOR
/**
 * @brief 接入 Unreal Data Validation，将领域校验错误报告给编辑器。
 * @param context 用于本次条件匹配的 `context` 标签或上下文。
 * @return 返回查询值、结构化结果或操作是否成功；失败语义由返回类型定义。
 */
EDataValidationResult ULRGameTuningSet::IsDataValid(FDataValidationContext& context) const
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
